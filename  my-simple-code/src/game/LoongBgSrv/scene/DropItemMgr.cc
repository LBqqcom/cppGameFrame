/*
 * DropItemMgr.cc
 *
 *  Created on: 2012-3-31
 *    
 */

#include <game/LoongBgSrv/scene/DropItemMgr.h>
#include <game/LoongBgSrv/scene/Scene.h>
#include <game/LoongBgSrv/BgPlayer.h>
#include <game/LoongBgSrv/Util.h>

#include <game/LoongBgSrv/base/ColonBuf.h>
#include <game/LoongBgSrv/config/ConfigMgr.h>
#include <game/LoongBgSrv/protocol/GameProtocol.h>

#include <string>

DropItemMgr::DropItemMgr(Scene* pScene):
	lastTime_(getCurTime()),
	pScene_(pScene)
{
	randomItemList_.resize(sRandomItemNum);
}

DropItemMgr::~DropItemMgr()
{
}


bool DropItemMgr::init()
{
	pointList_.clear();
	itemList_.clear();
	lastTime_ = getCurTime();

	randomItemList_.clear();
	for (int32 i = 0; i < sRandomItemNum; i++)
	{
		randomItemList_.push_back(i);
	}

	static std::string pointListStr = sConfigMgr.MainConfig.GetStringDefault("areas", "toolArea", "0:");
	static std::vector<struct point> sPointList;
	static bool initFlag = true;
	if (initFlag)
	{
		ColonBuf buf(pointListStr.c_str());
		int count;
		buf.GetIntDefault(count, 0);
		for (int i = 0; i < count; i++)
		{
			int16 x = buf.GetShort();
			int16 y = buf.GetShort();
			struct point p;
			p.x = x;
			p.y = y;
			sPointList.push_back(p);
		}
		initFlag = false;
	}

	pointList_ = sPointList;
	return true;
}

void DropItemMgr::run(int64 curTime)
{
	if (curTime - lastTime_ > sTenseconds)
	{
		if (itemList_.size() > sMaxItemNum)
		{
			LOG_TRACE << "DropItemMgr::run -- curTime: " << curTime
										<< " lastTime: " << lastTime_
										<< " leftTime: " << curTime - lastTime_
										<< " itemList size:" << itemList_.size();
			return;
		}

		if (randomItemList_.size() <= 0)
		{
		    LOG_TRACE << " have no item for drop!!!";
			return;
		}

		size_t pointNum = pointList_.size();
		if (pointNum == 0) return;

		int32  pointIndex = getRandomBetween(0, static_cast<int32>(pointNum - 1));
		int16 x = pointList_[pointIndex].x;
		int16 y = pointList_[pointIndex].y;
		int32  pointKey = MakeInt32(x, y);
		// 移除这个坐标点
		int32 lastIndex = static_cast<int32>(pointNum - 1);
		if (pointIndex != lastIndex)
		{
			pointList_[pointIndex].x = pointList_[lastIndex].x;
			pointList_[pointIndex].y = pointList_[lastIndex].y;
		}
		pointList_.pop_back();

		// 生成一个物品
		// 生命药水
		// 无敌药水
		// 腾云靴
		// 食人花种子
		//static const int32 itemList[] = {1, 2, 3, 4};
		//int32 itemIdIndex = getRandomBetween(0, sizeof(itemList) / sizeof(int32));
		int32 index = getRandomBetween(0, static_cast<int32>(randomItemList_.size() - 1));
		int32 itemId = randomItemList_[index] % sItemNum + 1;
		randomItemList_.erase(randomItemList_.begin()+index);

		LOG_TRACE << "DropItemMgr::run -- x: " << x
								<< " y: " << y
								<< " itemId: " << itemId
								<< " randomItem size: " << randomItemList_.size()
								<< " key: " << pointKey
								<< " pointList size: " << pointList_.size();

		struct DropItem item;
		item.itemId = itemId;
		item.point.x = x;
		item.point.y = y;
		itemList_.insert(std::pair<int32, struct DropItem>(pointKey, item));

		lastTime_ = curTime;

		// 广播给玩家知道, 随机生成了一个物品啦
		PacketBase op(client::OP_DROP_ITEM, 0);
		op.putInt32(itemId);
		op.putInt32(x);
		op.putInt32(y);
		pScene_->broadMsg(op);
	}
}

void DropItemMgr::shutdown()
{

}

bool DropItemMgr::pickUpItem(BgPlayer* player, int16 x, int16 y)
{
	int32  pointKey = MakeInt32(x, y);
	std::map<int32, struct DropItem>::iterator iter;
	iter = itemList_.find(pointKey);
	if (iter != itemList_.end())
	{
		int32 itemId =(iter->second).itemId;
		if (player->addItem(itemId))
		{
			itemList_.erase(pointKey);
			struct point p;
			p.x = x;
			p.y = y;
			pointList_.push_back(p);
			// 告诉客户端 这个坐标点的物品给人拾取啦
			int32 playerId = player->getId();
			PacketBase op(client::OP_PICKUP_ITEM, 0);
			op.putInt32(playerId);
			op.putInt32(itemId);
			op.putInt32(x);
			op.putInt32(y);
			pScene_->broadMsg(op);
		}
		// 把这个物品添加到玩家的背包中
		return true;
	}
	else
	{
		//告诉玩家 这个坐标点 没有这个物品哦
		PacketBase op(client::OP_PICKUP_ITEM, -1);
		player->sendPacket(op);
	}

	return false;
}


bool DropItemMgr::serialize(PacketBase& op)
{
	int32 itemNum = static_cast<int32>(itemList_.size());
	op.putInt32(itemNum);
	std::map<int32, struct DropItem>::iterator iter;
	for(iter = itemList_.begin(); iter != itemList_.end(); iter++)
	{
		int32 itemId = (iter->second).itemId;
		int16 x = (iter->second).point.x;
		int16 y = (iter->second).point.y;
		op.putInt32(x);
		op.putInt32(y);
		op.putInt32(itemId);
	}
	return true;
}
