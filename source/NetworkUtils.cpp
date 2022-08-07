#include "NetworkUtils.h"
#include "TargetEntity.h"
/*
static void updateEntity(Entity entity, BinaryInput inBuffer) {
	NetworkUtils::NetworkUpdateType type = (NetworkUtils::NetworkUpdateType)inBuffer.readUInt8();
}*/

void NetworkUtils::updateEntity(Array <GUniqueID> ignoreIDs, shared_ptr<G3D::Scene> scene, BinaryInput &inBuffer)
{
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

void NetworkUtils::createFrameUpdate(GUniqueID id, shared_ptr<Entity> entity, BinaryOutput &outBuffer)
{
	outBuffer.writeBytes(&id, sizeof(id));
	outBuffer.writeUInt8(NetworkUpdateType::REPLACE_FRAME);
	entity->frame().serialize(outBuffer);
}
