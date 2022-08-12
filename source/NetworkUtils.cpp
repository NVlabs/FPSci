#include "NetworkUtils.h"
#include "TargetEntity.h"

#include <enet/enet.h>
/*
static void updateEntity(Entity entity, BinaryInput inBuffer) {
	NetworkUtils::NetworkUpdateType type = (NetworkUtils::NetworkUpdateType)inBuffer.readUInt8();
}*/

void NetworkUtils::updateEntity(Array <GUniqueID> ignoreIDs, shared_ptr<G3D::Scene> scene, BinaryInput& inBuffer) {
	GUniqueID entity_id;
	inBuffer.readBytes(&entity_id, sizeof(entity_id));
	shared_ptr<NetworkedEntity> entity = (*scene).typedEntity<NetworkedEntity>(entity_id.toString16());
	if (ignoreIDs.contains(entity_id)) { // don't let the server move ignored entities
		entity = nullptr;
	}

	NetworkUtils::NetworkUpdateType type = (NetworkUtils::NetworkUpdateType)inBuffer.readUInt8();
	if (type == NOOP) {
		return;
	}
	else if (type == NetworkUpdateType::REPLACE_FRAME) {
		CoordinateFrame frame;
		frame.deserialize(inBuffer);
		if (entity != nullptr) {
			entity->setFrame(frame);
		}
	}
}

void NetworkUtils::updateEntity(shared_ptr<Entity> entity, BinaryInput& inBuffer) {
	NetworkUtils::NetworkUpdateType type = (NetworkUtils::NetworkUpdateType)inBuffer.readUInt8();
	if (type == NOOP) {
		return;
	}
	else if (type == NetworkUpdateType::REPLACE_FRAME) {
	CoordinateFrame frame;
	frame.deserialize(inBuffer);
	if (entity != nullptr) {
		entity->setFrame(frame);
	}
	}
}

void NetworkUtils::createFrameUpdate(GUniqueID id, shared_ptr<Entity> entity, BinaryOutput& outBuffer) {
	outBuffer.writeBytes(&id, sizeof(id));
	outBuffer.writeUInt8(NetworkUpdateType::REPLACE_FRAME);
	entity->frame().serialize(outBuffer);
}

void NetworkUtils::handleDestroyEntity(shared_ptr<G3D::Scene> scene, BinaryInput& inBuffer) {
	GUniqueID entity_id;
	inBuffer.readBytes(&entity_id, sizeof(entity_id));
	(*scene).removeEntity(entity_id.toString16());
}

ENetPacket* NetworkUtils::createDestroyEntityPacket(GUniqueID id) {
	BinaryOutput outBuffer;
	outBuffer.setEndian(G3D_BIG_ENDIAN);
	outBuffer.writeUInt8(NetworkUtils::MessageType::DESTROY_ENTITY);
	id.serialize(outBuffer);		// Send the GUID as a byte string

	return enet_packet_create((void*)outBuffer.getCArray(), outBuffer.length(), ENET_PACKET_FLAG_RELIABLE);
}

int NetworkUtils::createMoveClient(CFrame frame, ENetPeer* peer) {
	BinaryOutput outbuffer;
	outbuffer.setEndian(G3D::G3D_BIG_ENDIAN);
	outbuffer.writeUInt8(NetworkUtils::MOVE_CLIENT);
	outbuffer.writeUInt8(NetworkUpdateType::REPLACE_FRAME);
	frame.serialize(outbuffer);
	ENetPacket* packet = enet_packet_create((void*)outbuffer.getCArray(), outbuffer.length() + 1, ENET_PACKET_FLAG_RELIABLE);
	return enet_peer_send(peer, 0, packet);
}
