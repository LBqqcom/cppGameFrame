/*
 * Scene.cc
 *
 *  Created on: 2012-3-20
 *    
 */

#include <game/LoongBgSrv/scene/Scene.h>

#include <game/LoongBgSrv/protocol/GameProtocol.h>
#include <game/LoongBgSrv/BgPlayer.h>

#include <mysdk/protocol/kabu/codec/PacketBase.h>

Scene::Scene():
	sceneId_(0)
{

}

Scene::~Scene()
{
}

bool Scene::addPlayer(BgPlayer* player)
{
	assert(player != NULL);
	int32 playerId = player->getId();
	playerMgr_.insert(std::pair<int32, BgPlayer*>(playerId, player));

	PacketBase op(client::OP_ADD_PLAYER, 0);
	player->serialize(op);
	broadMsg(op);
	return true;
}

bool Scene::removePlayer(BgPlayer* player)
{
	assert(player != NULL);

	int32 playerId = player->getId();
	playerMgr_.erase(playerId);

	PacketBase op(client::OP_REMOVE_PLAYER, playerId);
	broadMsg(op);
	return true;
}

void Scene::broadMsg(PacketBase& op)
{
	std::map<int32, BgPlayer*>::iterator iter;
	for(iter = playerMgr_.begin(); iter != playerMgr_.end(); iter++)
	{
		BgPlayer* player = iter->second;
		if (player)
		{
			player->sendPacket(op);
		}
	}
}
bool Scene::init()
{
	return true;
}

void Scene::shutdown()
{
	std::map<int32, BgPlayer*>::iterator iter;
	for(iter = playerMgr_.begin(); iter != playerMgr_.end(); iter++)
	{
		BgPlayer* player = iter->second;
		if (player)
		{
			LOG_INFO << "============ Scene::shutdown() playerId: " << player->getId();
			delete player;
		}
	}
}

void Scene::run(uint32 curTime)
{
	std::map<int32, BgPlayer*>::iterator iter;
	for(iter = playerMgr_.begin(); iter != playerMgr_.end(); iter++)
	{
		BgPlayer* player = iter->second;
		if (player)
		{
			player->run(curTime);
		}
	}
}

BgPlayer* Scene::getPlayer(int playerId)
{
	std::map<int32, BgPlayer*>::iterator iter;
	iter = playerMgr_.find(playerId);
	if (iter != playerMgr_.end())
	{
		return iter->second;
	}
	return NULL;
}