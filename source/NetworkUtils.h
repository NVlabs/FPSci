#pragma once
#include <G3D/G3D.h>
#include <enet/enet.h>

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

			#NYI
			Type DESTROY_ENTITY:
			UInt8: type (DESTROY_ENTITY)
			GUID: object ID

			Type REGISTER_CLIENT:
			UInt8: type (REGISTER_CLIENT)
			GUID: player's ID
			UInt16: client's port
			? String: player metadata

			Type CLIENT_REGISTRATION_REPLY:
			UInt8: type (CLIENT_REGISTRATION_REPLY)
			GUID: player's ID
			UInt8: status [0 = success, 1 = Failure, ....]

			Type HANDSHAKE
			UInt8: type (HANDSHAKE)

			Type HANDSHAKE_REPLY
			UInt8: type (HANDSHAKE_REPLY)

			#NYI
			Type MOVE_CLIENT:
			UInt 8: type (MOVE_CLIENT)
			CFrame: new position

			Type CLIENT_DISCONNECTED: ?????

			Type REGISTER_HIT:


	*/

class NetworkUtils
{
public:
	enum MessageType {
		CONTROL_MESSAGE,
		UPDATE_MESSAGE,

		BATCH_ENTITY_UPDATE,
		CREATE_ENTITY,
		DESTROY_ENTITY,
		MOVE_CLIENT,


		REGISTER_CLIENT,
		CLIENT_REGISTRATION_REPLY,

		HANDSHAKE,
		HANDSHAKE_REPLY
	};

	enum NetworkUpdateType {
		NOOP,
		REPLACE_FRAME,
	};

	static void updateEntity(Array <GUniqueID> ignoreIDs, shared_ptr<G3D::Scene> scene, BinaryInput& inBuffer);
	static void createFrameUpdate(GUniqueID id, shared_ptr<Entity> entity, BinaryOutput& outBuffer);

	static void handleDestroyEntity(shared_ptr<G3D::Scene> scene, BinaryInput& inBuffer);
	static ENetPacket* createDestroyEntityPacket(GUniqueID id);
};

