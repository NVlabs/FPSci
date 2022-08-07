#pragma once
#include <G3D/G3D.h>

#define BATCH_UPDATE_COUNT_POSITION 1

/*
			PACKET STRUCTURE:
			UInt8: type
			...

			Type BATCH_ENTITY_UPDATE:
			UInt8: type (BATCH_ENTITY_UPDATE)
			UInt8: object_count # number of frames contained in this packet
			<DATA> * n: Opaque view of data, written to and from with NetworkUtils
			[DEPRECATED] GUID * n: ID of object
			[DEPRECATED] * n: CFrames of objects

			Type CREATE_ENTITY:
			UInt8: type (CREATE_ENTITY)
			GUID: object ID
			Uint8: Entity Type
			... : misc. data for constructor, dependent on Entity type

			Type REGISTER_CLIENT:
			GUID: player's ID
			UInt16: client's port
			? String: player metadata

			Type CLIENT_REGISTRATION_REPLY:
			GUID: player's ID
			UInt8: status [0 = success, 1 = Failure, ....]

			Type HANDSHAKE

			Type HANDSHAKE_REPLY

	*/

class NetworkUtils
{
public:
	enum NetworkUpdateType {
		NOOP,
		REPLACE_FRAME,
	};
	static void updateEntity(Array <GUniqueID> ignoreIDs, shared_ptr<G3D::Scene> scene, BinaryInput &inBuffer);

	static void createFrameUpdate(GUniqueID id, shared_ptr<Entity> entity, BinaryOutput &outBuffer);
};

