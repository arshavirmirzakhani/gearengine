// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#include "joint.h"

#include "body.h"
#include "contact.h"
#include "core.h"
#include "island.h"
#include "shape.h"
#include "solver.h"
#include "solver_set.h"
#include "world.h"

// needed for dll export
#include "box2d/box2d.h"

#include <stddef.h>
#include <string.h>

B2_ARRAY_SOURCE( b2Joint, b2Joint )
B2_ARRAY_SOURCE( b2JointSim, b2JointSim )

b2DistanceJointDef b2DefaultDistanceJointDef( void )
{
	b2DistanceJointDef def = { 0 };
	def.length = 1.0f;
	def.maxLength = B2_HUGE;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2MotorJointDef b2DefaultMotorJointDef( void )
{
	b2MotorJointDef def = { 0 };
	def.maxForce = 1.0f;
	def.maxTorque = 1.0f;
	def.correctionFactor = 0.3f;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2MouseJointDef b2DefaultMouseJointDef( void )
{
	b2MouseJointDef def = { 0 };
	def.hertz = 4.0f;
	def.dampingRatio = 1.0f;
	def.maxForce = 1.0f;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2FilterJointDef b2DefaultFilterJointDef( void )
{
	b2FilterJointDef def = { 0 };
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2PrismaticJointDef b2DefaultPrismaticJointDef( void )
{
	b2PrismaticJointDef def = { 0 };
	def.localAxisA = (b2Vec2){ 1.0f, 0.0f };
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2RevoluteJointDef b2DefaultRevoluteJointDef( void )
{
	b2RevoluteJointDef def = { 0 };
	def.drawSize = 0.25f;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2WeldJointDef b2DefaultWeldJointDef( void )
{
	b2WeldJointDef def = { 0 };
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2WheelJointDef b2DefaultWheelJointDef( void )
{
	b2WheelJointDef def = { 0 };
	def.localAxisA.y = 1.0f;
	def.enableSpring = true;
	def.hertz = 1.0f;
	def.dampingRatio = 0.7f;
	def.internalValue = B2_SECRET_COOKIE;
	return def;
}

b2ExplosionDef b2DefaultExplosionDef( void )
{
	b2ExplosionDef def = { 0 };
	def.maskBits = B2_DEFAULT_MASK_BITS;
	return def;
}

b2Joint* b2GetJointFullId( b2World* world, b2JointId jointId )
{
	int id = jointId.index1 - 1;
	b2Joint* joint = b2JointArray_Get( &world->joints, id );
	B2_ASSERT( joint->jointId == id && joint->generation == jointId.generation );
	return joint;
}

b2JointSim* b2GetJointSim( b2World* world, b2Joint* joint )
{
	if ( joint->setIndex == b2_awakeSet )
	{
		B2_ASSERT( 0 <= joint->colorIndex && joint->colorIndex < B2_GRAPH_COLOR_COUNT );
		b2GraphColor* color = world->constraintGraph.colors + joint->colorIndex;
		return b2JointSimArray_Get( &color->jointSims, joint->localIndex );
	}

	b2SolverSet* set = b2SolverSetArray_Get( &world->solverSets, joint->setIndex );
	return b2JointSimArray_Get( &set->jointSims, joint->localIndex );
}

b2JointSim* b2GetJointSimCheckType( b2JointId jointId, b2JointType type )
{
	B2_UNUSED( type );

	b2World* world = b2GetWorld( jointId.world0 );
	B2_ASSERT( world->locked == false );
	if ( world->locked )
	{
		return NULL;
	}

	b2Joint* joint = b2GetJointFullId( world, jointId );
	B2_ASSERT( joint->type == type );
	b2JointSim* jointSim = b2GetJointSim( world, joint );
	B2_ASSERT( jointSim->type == type );
	return jointSim;
}

typedef struct b2JointPair
{
	b2Joint* joint;
	b2JointSim* jointSim;
} b2JointPair;

static b2JointPair b2CreateJoint( b2World* world, b2Body* bodyA, b2Body* bodyB, void* userData, float drawSize, b2JointType type,
								  bool collideConnected )
{
	int bodyIdA = bodyA->id;
	int bodyIdB = bodyB->id;
	int maxSetIndex = b2MaxInt( bodyA->setIndex, bodyB->setIndex );

	// Create joint id and joint
	int jointId = b2AllocId( &world->jointIdPool );
	if ( jointId == world->joints.count )
	{
		b2JointArray_Push( &world->joints, (b2Joint){ 0 } );
	}

	b2Joint* joint = b2JointArray_Get( &world->joints, jointId );
	joint->jointId = jointId;
	joint->userData = userData;
	joint->generation += 1;
	joint->setIndex = B2_NULL_INDEX;
	joint->colorIndex = B2_NULL_INDEX;
	joint->localIndex = B2_NULL_INDEX;
	joint->islandId = B2_NULL_INDEX;
	joint->islandPrev = B2_NULL_INDEX;
	joint->islandNext = B2_NULL_INDEX;
	joint->drawSize = drawSize;
	joint->type = type;
	joint->collideConnected = collideConnected;
	joint->isMarked = false;

	// Doubly linked list on bodyA
	joint->edges[0].bodyId = bodyIdA;
	joint->edges[0].prevKey = B2_NULL_INDEX;
	joint->edges[0].nextKey = bodyA->headJointKey;

	int keyA = ( jointId << 1 ) | 0;
	if ( bodyA->headJointKey != B2_NULL_INDEX )
	{
		b2Joint* jointA = b2JointArray_Get( &world->joints, bodyA->headJointKey >> 1 );
		b2JointEdge* edgeA = jointA->edges + ( bodyA->headJointKey & 1 );
		edgeA->prevKey = keyA;
	}
	bodyA->headJointKey = keyA;
	bodyA->jointCount += 1;

	// Doubly linked list on bodyB
	joint->edges[1].bodyId = bodyIdB;
	joint->edges[1].prevKey = B2_NULL_INDEX;
	joint->edges[1].nextKey = bodyB->headJointKey;

	int keyB = ( jointId << 1 ) | 1;
	if ( bodyB->headJointKey != B2_NULL_INDEX )
	{
		b2Joint* jointB = b2JointArray_Get( &world->joints, bodyB->headJointKey >> 1 );
		b2JointEdge* edgeB = jointB->edges + ( bodyB->headJointKey & 1 );
		edgeB->prevKey = keyB;
	}
	bodyB->headJointKey = keyB;
	bodyB->jointCount += 1;

	b2JointSim* jointSim;

	if ( bodyA->setIndex == b2_disabledSet || bodyB->setIndex == b2_disabledSet )
	{
		// if either body is disabled, create in disabled set
		b2SolverSet* set = b2SolverSetArray_Get( &world->solverSets, b2_disabledSet );
		joint->setIndex = b2_disabledSet;
		joint->localIndex = set->jointSims.count;

		jointSim = b2JointSimArray_Add( &set->jointSims );
		memset( jointSim, 0, sizeof( b2JointSim ) );

		jointSim->jointId = jointId;
		jointSim->bodyIdA = bodyIdA;
		jointSim->bodyIdB = bodyIdB;
	}
	else if ( bodyA->setIndex == b2_staticSet && bodyB->setIndex == b2_staticSet )
	{
		// joint is connecting static bodies
		b2SolverSet* set = b2SolverSetArray_Get( &world->solverSets, b2_staticSet );
		joint->setIndex = b2_staticSet;
		joint->localIndex = set->jointSims.count;

		jointSim = b2JointSimArray_Add( &set->jointSims );
		memset( jointSim, 0, sizeof( b2JointSim ) );

		jointSim->jointId = jointId;
		jointSim->bodyIdA = bodyIdA;
		jointSim->bodyIdB = bodyIdB;
	}
	else if ( bodyA->setIndex == b2_awakeSet || bodyB->setIndex == b2_awakeSet )
	{
		// if either body is sleeping, wake it
		if ( maxSetIndex >= b2_firstSleepingSet )
		{
			b2WakeSolverSet( world, maxSetIndex );
		}

		joint->setIndex = b2_awakeSet;

		jointSim = b2CreateJointInGraph( world, joint );
		jointSim->jointId = jointId;
		jointSim->bodyIdA = bodyIdA;
		jointSim->bodyIdB = bodyIdB;
	}
	else
	{
		// joint connected between sleeping and/or static bodies
		B2_ASSERT( bodyA->setIndex >= b2_firstSleepingSet || bodyB->setIndex >= b2_firstSleepingSet );
		B2_ASSERT( bodyA->setIndex != b2_staticSet || bodyB->setIndex != b2_staticSet );

		// joint should go into the sleeping set (not static set)
		int setIndex = maxSetIndex;

		b2SolverSet* set = b2SolverSetArray_Get( &world->solverSets, setIndex );
		joint->setIndex = setIndex;
		joint->localIndex = set->jointSims.count;

		jointSim = b2JointSimArray_Add( &set->jointSims );
		memset( jointSim, 0, sizeof( b2JointSim ) );

		jointSim->jointId = jointId;
		jointSim->bodyIdA = bodyIdA;
		jointSim->bodyIdB = bodyIdB;

		if ( bodyA->setIndex != bodyB->setIndex && bodyA->setIndex >= b2_firstSleepingSet &&
			 bodyB->setIndex >= b2_firstSleepingSet )
		{
			// merge sleeping sets
			b2MergeSolverSets( world, bodyA->setIndex, bodyB->setIndex );
			B2_ASSERT( bodyA->setIndex == bodyB->setIndex );

			// fix potentially invalid set index
			setIndex = bodyA->setIndex;

			b2SolverSet* mergedSet = b2SolverSetArray_Get( &world->solverSets, setIndex );

			// Careful! The joint sim pointer was orphaned by the set merge.
			jointSim = b2JointSimArray_Get( &mergedSet->jointSims, joint->localIndex );
		}

		B2_ASSERT( joint->setIndex == setIndex );
	}

	jointSim->constraintHertz = B2_JOINT_CONSTRAINT_HERTZ;
	jointSim->constraintDampingRatio = B2_JOINT_CONSTRAINT_DAMPING_RATIO;
	jointSim->constraintSoftness = (b2Softness){
		.biasRate = 0.0f,
		.massScale = 1.0f,
		.impulseScale = 0.0f,
	};

	B2_ASSERT( jointSim->jointId == jointId );
	B2_ASSERT( jointSim->bodyIdA == bodyIdA );
	B2_ASSERT( jointSim->bodyIdB == bodyIdB );

	if ( joint->setIndex > b2_disabledSet )
	{
		// Add edge to island graph
		bool mergeIslands = true;
		b2LinkJoint( world, joint, mergeIslands );
	}

	b2ValidateSolverSets( world );

	return (b2JointPair){ joint, jointSim };
}

static void b2DestroyContactsBetweenBodies( b2World* world, b2Body* bodyA, b2Body* bodyB )
{
	int contactKey;
	int otherBodyId;

	// use the smaller of the two contact lists
	if ( bodyA->contactCount < bodyB->contactCount )
	{
		contactKey = bodyA->headContactKey;
		otherBodyId = bodyB->id;
	}
	else
	{
		contactKey = bodyB->headContactKey;
		otherBodyId = bodyA->id;
	}

	// no need to wake bodies when a joint removes collision between them
	bool wakeBodies = false;

	// destroy the contacts
	while ( contactKey != B2_NULL_INDEX )
	{
		int contactId = contactKey >> 1;
		int edgeIndex = contactKey & 1;

		b2Contact* contact = b2ContactArray_Get( &world->contacts, contactId );
		contactKey = contact->edges[edgeIndex].nextKey;

		int otherEdgeIndex = edgeIndex ^ 1;
		if ( contact->edges[otherEdgeIndex].bodyId == otherBodyId )
		{
			// Careful, this removes the contact from the current doubly linked list
			b2DestroyContact( world, contact, wakeBodies );
		}
	}

	b2ValidateSolverSets( world );
}

b2JointId b2CreateDistanceJoint( b2WorldId worldId, const b2DistanceJointDef* def )
{
	B2_CHECK_DEF( def );
	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	B2_ASSERT( b2Body_IsValid( def->bodyIdA ) );
	B2_ASSERT( b2Body_IsValid( def->bodyIdB ) );
	B2_ASSERT( b2IsValidFloat( def->length ) && def->length > 0.0f );

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_distanceJoint, def->collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_distanceJoint;
	joint->localOriginAnchorA = def->localAnchorA;
	joint->localOriginAnchorB = def->localAnchorB;

	b2DistanceJoint empty = { 0 };
	joint->distanceJoint = empty;
	joint->distanceJoint.length = b2MaxFloat( def->length, B2_LINEAR_SLOP );
	joint->distanceJoint.hertz = def->hertz;
	joint->distanceJoint.dampingRatio = def->dampingRatio;
	joint->distanceJoint.minLength = b2MaxFloat( def->minLength, B2_LINEAR_SLOP );
	joint->distanceJoint.maxLength = b2MaxFloat( def->minLength, def->maxLength );
	joint->distanceJoint.maxMotorForce = def->maxMotorForce;
	joint->distanceJoint.motorSpeed = def->motorSpeed;
	joint->distanceJoint.enableSpring = def->enableSpring;
	joint->distanceJoint.enableLimit = def->enableLimit;
	joint->distanceJoint.enableMotor = def->enableMotor;
	joint->distanceJoint.impulse = 0.0f;
	joint->distanceJoint.lowerImpulse = 0.0f;
	joint->distanceJoint.upperImpulse = 0.0f;
	joint->distanceJoint.motorImpulse = 0.0f;

	// If the joint prevents collisions, then destroy all contacts between attached bodies
	if ( def->collideConnected == false )
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreateMotorJoint( b2WorldId worldId, const b2MotorJointDef* def )
{
	B2_CHECK_DEF( def );
	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_motorJoint, def->collideConnected );
	b2JointSim* joint = pair.jointSim;

	joint->type = b2_motorJoint;
	joint->localOriginAnchorA = (b2Vec2){ 0.0f, 0.0f };
	joint->localOriginAnchorB = (b2Vec2){ 0.0f, 0.0f };
	joint->motorJoint = (b2MotorJoint){ 0 };
	joint->motorJoint.linearOffset = def->linearOffset;
	joint->motorJoint.angularOffset = def->angularOffset;
	joint->motorJoint.maxForce = def->maxForce;
	joint->motorJoint.maxTorque = def->maxTorque;
	joint->motorJoint.correctionFactor = b2ClampFloat( def->correctionFactor, 0.0f, 1.0f );

	// If the joint prevents collisions, then destroy all contacts between attached bodies
	if ( def->collideConnected == false )
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreateMouseJoint( b2WorldId worldId, const b2MouseJointDef* def )
{
	B2_CHECK_DEF( def );
	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2Transform transformA = b2GetBodyTransformQuick( world, bodyA );
	b2Transform transformB = b2GetBodyTransformQuick( world, bodyB );

	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_mouseJoint, def->collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_mouseJoint;
	joint->localOriginAnchorA = b2InvTransformPoint( transformA, def->target );
	joint->localOriginAnchorB = b2InvTransformPoint( transformB, def->target );

	b2MouseJoint empty = { 0 };
	joint->mouseJoint = empty;
	joint->mouseJoint.targetA = def->target;
	joint->mouseJoint.hertz = def->hertz;
	joint->mouseJoint.dampingRatio = def->dampingRatio;
	joint->mouseJoint.maxForce = def->maxForce;

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreateFilterJoint( b2WorldId worldId, const b2FilterJointDef* def )
{
	B2_CHECK_DEF( def );
	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	bool collideConnected = false;
	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_filterJoint, collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_filterJoint;
	joint->localOriginAnchorA = b2Vec2_zero;
	joint->localOriginAnchorB = b2Vec2_zero;

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreateRevoluteJoint( b2WorldId worldId, const b2RevoluteJointDef* def )
{
	B2_CHECK_DEF( def );
	B2_ASSERT( def->lowerAngle <= def->upperAngle );
	B2_ASSERT( def->lowerAngle >= -0.99f * B2_PI );
	B2_ASSERT( def->upperAngle <= 0.99f * B2_PI );

	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2JointPair pair =
		b2CreateJoint( world, bodyA, bodyB, def->userData, def->drawSize, b2_revoluteJoint, def->collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_revoluteJoint;
	joint->localOriginAnchorA = def->localAnchorA;
	joint->localOriginAnchorB = def->localAnchorB;

	b2RevoluteJoint empty = { 0 };
	joint->revoluteJoint = empty;

	joint->revoluteJoint.referenceAngle = b2ClampFloat( def->referenceAngle, -B2_PI, B2_PI );
	joint->revoluteJoint.targetAngle = b2ClampFloat( def->targetAngle, -B2_PI, B2_PI );
	joint->revoluteJoint.hertz = def->hertz;
	joint->revoluteJoint.dampingRatio = def->dampingRatio;
	joint->revoluteJoint.lowerAngle = def->lowerAngle;
	joint->revoluteJoint.upperAngle = def->upperAngle;
	joint->revoluteJoint.maxMotorTorque = def->maxMotorTorque;
	joint->revoluteJoint.motorSpeed = def->motorSpeed;
	joint->revoluteJoint.enableSpring = def->enableSpring;
	joint->revoluteJoint.enableLimit = def->enableLimit;
	joint->revoluteJoint.enableMotor = def->enableMotor;

	// If the joint prevents collisions, then destroy all contacts between attached bodies
	if ( def->collideConnected == false )
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreatePrismaticJoint( b2WorldId worldId, const b2PrismaticJointDef* def )
{
	B2_CHECK_DEF( def );
	B2_ASSERT( def->lowerTranslation <= def->upperTranslation );

	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_prismaticJoint, def->collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_prismaticJoint;
	joint->localOriginAnchorA = def->localAnchorA;
	joint->localOriginAnchorB = def->localAnchorB;

	b2PrismaticJoint empty = { 0 };
	joint->prismaticJoint = empty;

	joint->prismaticJoint.localAxisA = b2Normalize( def->localAxisA );
	joint->prismaticJoint.referenceAngle = def->referenceAngle;
	joint->prismaticJoint.targetTranslation = def->targetTranslation;
	joint->prismaticJoint.hertz = def->hertz;
	joint->prismaticJoint.dampingRatio = def->dampingRatio;
	joint->prismaticJoint.lowerTranslation = def->lowerTranslation;
	joint->prismaticJoint.upperTranslation = def->upperTranslation;
	joint->prismaticJoint.maxMotorForce = def->maxMotorForce;
	joint->prismaticJoint.motorSpeed = def->motorSpeed;
	joint->prismaticJoint.enableSpring = def->enableSpring;
	joint->prismaticJoint.enableLimit = def->enableLimit;
	joint->prismaticJoint.enableMotor = def->enableMotor;

	// If the joint prevents collisions, then destroy all contacts between attached bodies
	if ( def->collideConnected == false )
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreateWeldJoint( b2WorldId worldId, const b2WeldJointDef* def )
{
	B2_CHECK_DEF( def );
	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_weldJoint, def->collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_weldJoint;
	joint->localOriginAnchorA = def->localAnchorA;
	joint->localOriginAnchorB = def->localAnchorB;

	b2WeldJoint empty = { 0 };
	joint->weldJoint = empty;
	joint->weldJoint.referenceAngle = def->referenceAngle;
	joint->weldJoint.linearHertz = def->linearHertz;
	joint->weldJoint.linearDampingRatio = def->linearDampingRatio;
	joint->weldJoint.angularHertz = def->angularHertz;
	joint->weldJoint.angularDampingRatio = def->angularDampingRatio;
	joint->weldJoint.linearImpulse = b2Vec2_zero;
	joint->weldJoint.angularImpulse = 0.0f;

	// If the joint prevents collisions, then destroy all contacts between attached bodies
	if ( def->collideConnected == false )
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

b2JointId b2CreateWheelJoint( b2WorldId worldId, const b2WheelJointDef* def )
{
	B2_CHECK_DEF( def );
	B2_ASSERT( def->lowerTranslation <= def->upperTranslation );

	b2World* world = b2GetWorldFromId( worldId );

	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return (b2JointId){ 0 };
	}

	b2Body* bodyA = b2GetBodyFullId( world, def->bodyIdA );
	b2Body* bodyB = b2GetBodyFullId( world, def->bodyIdB );

	b2JointPair pair = b2CreateJoint( world, bodyA, bodyB, def->userData, 1.0f, b2_wheelJoint, def->collideConnected );

	b2JointSim* joint = pair.jointSim;
	joint->type = b2_wheelJoint;
	joint->localOriginAnchorA = def->localAnchorA;
	joint->localOriginAnchorB = def->localAnchorB;

	joint->wheelJoint = (b2WheelJoint){ 0 };
	joint->wheelJoint.localAxisA = b2Normalize( def->localAxisA );
	joint->wheelJoint.perpMass = 0.0f;
	joint->wheelJoint.axialMass = 0.0f;
	joint->wheelJoint.motorImpulse = 0.0f;
	joint->wheelJoint.lowerImpulse = 0.0f;
	joint->wheelJoint.upperImpulse = 0.0f;
	joint->wheelJoint.lowerTranslation = def->lowerTranslation;
	joint->wheelJoint.upperTranslation = def->upperTranslation;
	joint->wheelJoint.maxMotorTorque = def->maxMotorTorque;
	joint->wheelJoint.motorSpeed = def->motorSpeed;
	joint->wheelJoint.hertz = def->hertz;
	joint->wheelJoint.dampingRatio = def->dampingRatio;
	joint->wheelJoint.enableSpring = def->enableSpring;
	joint->wheelJoint.enableLimit = def->enableLimit;
	joint->wheelJoint.enableMotor = def->enableMotor;

	// If the joint prevents collisions, then destroy all contacts between attached bodies
	if ( def->collideConnected == false )
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}

	b2JointId jointId = { joint->jointId + 1, world->worldId, pair.joint->generation };
	return jointId;
}

void b2DestroyJointInternal( b2World* world, b2Joint* joint, bool wakeBodies )
{
	int jointId = joint->jointId;

	b2JointEdge* edgeA = joint->edges + 0;
	b2JointEdge* edgeB = joint->edges + 1;

	int idA = edgeA->bodyId;
	int idB = edgeB->bodyId;
	b2Body* bodyA = b2BodyArray_Get( &world->bodies, idA );
	b2Body* bodyB = b2BodyArray_Get( &world->bodies, idB );

	// Remove from body A
	if ( edgeA->prevKey != B2_NULL_INDEX )
	{
		b2Joint* prevJoint = b2JointArray_Get( &world->joints, edgeA->prevKey >> 1 );
		b2JointEdge* prevEdge = prevJoint->edges + ( edgeA->prevKey & 1 );
		prevEdge->nextKey = edgeA->nextKey;
	}

	if ( edgeA->nextKey != B2_NULL_INDEX )
	{
		b2Joint* nextJoint = b2JointArray_Get( &world->joints, edgeA->nextKey >> 1 );
		b2JointEdge* nextEdge = nextJoint->edges + ( edgeA->nextKey & 1 );
		nextEdge->prevKey = edgeA->prevKey;
	}

	int edgeKeyA = ( jointId << 1 ) | 0;
	if ( bodyA->headJointKey == edgeKeyA )
	{
		bodyA->headJointKey = edgeA->nextKey;
	}

	bodyA->jointCount -= 1;

	// Remove from body B
	if ( edgeB->prevKey != B2_NULL_INDEX )
	{
		b2Joint* prevJoint = b2JointArray_Get( &world->joints, edgeB->prevKey >> 1 );
		b2JointEdge* prevEdge = prevJoint->edges + ( edgeB->prevKey & 1 );
		prevEdge->nextKey = edgeB->nextKey;
	}

	if ( edgeB->nextKey != B2_NULL_INDEX )
	{
		b2Joint* nextJoint = b2JointArray_Get( &world->joints, edgeB->nextKey >> 1 );
		b2JointEdge* nextEdge = nextJoint->edges + ( edgeB->nextKey & 1 );
		nextEdge->prevKey = edgeB->prevKey;
	}

	int edgeKeyB = ( jointId << 1 ) | 1;
	if ( bodyB->headJointKey == edgeKeyB )
	{
		bodyB->headJointKey = edgeB->nextKey;
	}

	bodyB->jointCount -= 1;

	if ( joint->islandId != B2_NULL_INDEX )
	{
		B2_ASSERT( joint->setIndex > b2_disabledSet );
		b2UnlinkJoint( world, joint );
	}
	else
	{
		B2_ASSERT( joint->setIndex <= b2_disabledSet );
	}

	// Remove joint from solver set that owns it
	int setIndex = joint->setIndex;
	int localIndex = joint->localIndex;

	if ( setIndex == b2_awakeSet )
	{
		b2RemoveJointFromGraph( world, joint->edges[0].bodyId, joint->edges[1].bodyId, joint->colorIndex, localIndex );
	}
	else
	{
		b2SolverSet* set = b2SolverSetArray_Get( &world->solverSets, setIndex );
		int movedIndex = b2JointSimArray_RemoveSwap( &set->jointSims, localIndex );
		if ( movedIndex != B2_NULL_INDEX )
		{
			// Fix moved joint
			b2JointSim* movedJointSim = set->jointSims.data + localIndex;
			int movedId = movedJointSim->jointId;
			b2Joint* movedJoint = b2JointArray_Get( &world->joints, movedId );
			B2_ASSERT( movedJoint->localIndex == movedIndex );
			movedJoint->localIndex = localIndex;
		}
	}

	// Free joint and id (preserve joint generation)
	joint->setIndex = B2_NULL_INDEX;
	joint->localIndex = B2_NULL_INDEX;
	joint->colorIndex = B2_NULL_INDEX;
	joint->jointId = B2_NULL_INDEX;
	b2FreeId( &world->jointIdPool, jointId );

	if ( wakeBodies )
	{
		b2WakeBody( world, bodyA );
		b2WakeBody( world, bodyB );
	}

	b2ValidateSolverSets( world );
}

void b2DestroyJoint( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	B2_ASSERT( world->locked == false );

	if ( world->locked )
	{
		return;
	}

	b2Joint* joint = b2GetJointFullId( world, jointId );

	b2DestroyJointInternal( world, joint, true );
}

b2JointType b2Joint_GetType( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	return joint->type;
}

b2BodyId b2Joint_GetBodyA( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	return b2MakeBodyId( world, joint->edges[0].bodyId );
}

b2BodyId b2Joint_GetBodyB( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	return b2MakeBodyId( world, joint->edges[1].bodyId );
}

b2WorldId b2Joint_GetWorld( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	return (b2WorldId){ jointId.world0 + 1, world->generation };
}

void b2Joint_SetLocalAnchorA( b2JointId jointId, b2Vec2 localAnchor )
{
	B2_ASSERT( b2IsValidVec2( localAnchor ) );

	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );
	jointSim->localOriginAnchorA = localAnchor;
}

b2Vec2 b2Joint_GetLocalAnchorA( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );
	return jointSim->localOriginAnchorA;
}

void b2Joint_SetLocalAnchorB( b2JointId jointId, b2Vec2 localAnchor )
{
	B2_ASSERT( b2IsValidVec2( localAnchor ) );

	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );
	jointSim->localOriginAnchorB = localAnchor;
}

b2Vec2 b2Joint_GetLocalAnchorB( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );
	return jointSim->localOriginAnchorB;
}

void b2Joint_SetReferenceAngle( b2JointId jointId, float angleInRadians )
{
	B2_ASSERT( b2IsValidFloat( angleInRadians ) );

	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );

	switch ( joint->type )
	{
		case b2_prismaticJoint:
			jointSim->prismaticJoint.referenceAngle = angleInRadians;
			break;

		case b2_revoluteJoint:
			jointSim->revoluteJoint.referenceAngle = angleInRadians;
			break;

		case b2_weldJoint:
			jointSim->weldJoint.referenceAngle = angleInRadians;
			break;

		default:
			break;
	}
}

float b2Joint_GetReferenceAngle( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );

	switch ( joint->type )
	{
		case b2_prismaticJoint:
			return jointSim->prismaticJoint.referenceAngle;

		case b2_revoluteJoint:
			return jointSim->revoluteJoint.referenceAngle;

		case b2_weldJoint:
			return jointSim->weldJoint.referenceAngle;

		default:
			return 0.0f;
	}
}

void b2Joint_SetLocalAxisA( b2JointId jointId, b2Vec2 localAxis )
{
	B2_ASSERT( b2IsValidVec2( localAxis ) );
	B2_ASSERT( b2IsNormalized( localAxis ) );

	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );

	switch ( joint->type )
	{
		case b2_prismaticJoint:
			jointSim->prismaticJoint.localAxisA = localAxis;
			break;

		case b2_wheelJoint:
			jointSim->wheelJoint.localAxisA = localAxis;
			break;

		default:
			break;
	}
}

b2Vec2 b2Joint_GetLocalAxisA( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* jointSim = b2GetJointSim( world, joint );

	switch ( joint->type )
	{
		case b2_prismaticJoint:
			return jointSim->prismaticJoint.localAxisA;

		case b2_wheelJoint:
			return jointSim->wheelJoint.localAxisA;

		default:
			return b2Vec2_zero;
	}
}

void b2Joint_SetCollideConnected( b2JointId jointId, bool shouldCollide )
{
	b2World* world = b2GetWorldLocked( jointId.world0 );
	if ( world == NULL )
	{
		return;
	}

	b2Joint* joint = b2GetJointFullId( world, jointId );
	if ( joint->collideConnected == shouldCollide )
	{
		return;
	}

	joint->collideConnected = shouldCollide;

	b2Body* bodyA = b2BodyArray_Get( &world->bodies, joint->edges[0].bodyId );
	b2Body* bodyB = b2BodyArray_Get( &world->bodies, joint->edges[1].bodyId );

	if ( shouldCollide )
	{
		// need to tell the broad-phase to look for new pairs for one of the
		// two bodies. Pick the one with the fewest shapes.
		int shapeCountA = bodyA->shapeCount;
		int shapeCountB = bodyB->shapeCount;

		int shapeId = shapeCountA < shapeCountB ? bodyA->headShapeId : bodyB->headShapeId;
		while ( shapeId != B2_NULL_INDEX )
		{
			b2Shape* shape = b2ShapeArray_Get( &world->shapes, shapeId );

			if ( shape->proxyKey != B2_NULL_INDEX )
			{
				b2BufferMove( &world->broadPhase, shape->proxyKey );
			}

			shapeId = shape->nextShapeId;
		}
	}
	else
	{
		b2DestroyContactsBetweenBodies( world, bodyA, bodyB );
	}
}

bool b2Joint_GetCollideConnected( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	return joint->collideConnected;
}

void b2Joint_SetUserData( b2JointId jointId, void* userData )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	joint->userData = userData;
}

void* b2Joint_GetUserData( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	return joint->userData;
}

void b2Joint_WakeBodies( b2JointId jointId )
{
	b2World* world = b2GetWorldLocked( jointId.world0 );
	if ( world == NULL )
	{
		return;
	}

	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2Body* bodyA = b2BodyArray_Get( &world->bodies, joint->edges[0].bodyId );
	b2Body* bodyB = b2BodyArray_Get( &world->bodies, joint->edges[1].bodyId );

	b2WakeBody( world, bodyA );
	b2WakeBody( world, bodyB );
}

b2Vec2 b2Joint_GetConstraintForce( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* base = b2GetJointSim( world, joint );

	switch ( joint->type )
	{
		case b2_distanceJoint:
			return b2GetDistanceJointForce( world, base );

		case b2_motorJoint:
			return b2GetMotorJointForce( world, base );

		case b2_mouseJoint:
			return b2GetMouseJointForce( world, base );

		case b2_filterJoint:
			return b2Vec2_zero;

		case b2_prismaticJoint:
			return b2GetPrismaticJointForce( world, base );

		case b2_revoluteJoint:
			return b2GetRevoluteJointForce( world, base );

		case b2_weldJoint:
			return b2GetWeldJointForce( world, base );

		case b2_wheelJoint:
			return b2GetWheelJointForce( world, base );

		default:
			B2_ASSERT( false );
			return b2Vec2_zero;
	}
}

float b2Joint_GetConstraintTorque( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* base = b2GetJointSim( world, joint );

	switch ( joint->type )
	{
		case b2_distanceJoint:
			return 0.0f;

		case b2_motorJoint:
			return b2GetMotorJointTorque( world, base );

		case b2_mouseJoint:
			return b2GetMouseJointTorque( world, base );

		case b2_filterJoint:
			return 0.0f;

		case b2_prismaticJoint:
			return b2GetPrismaticJointTorque( world, base );

		case b2_revoluteJoint:
			return b2GetRevoluteJointTorque( world, base );

		case b2_weldJoint:
			return b2GetWeldJointTorque( world, base );

		case b2_wheelJoint:
			return b2GetWheelJointTorque( world, base );

		default:
			B2_ASSERT( false );
			return 0.0f;
	}
}

float b2Joint_GetLinearSeparation( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* base = b2GetJointSim( world, joint );

	b2Transform xfA = b2GetBodyTransform( world, joint->edges[0].bodyId );
	b2Transform xfB = b2GetBodyTransform( world, joint->edges[1].bodyId );

	b2Vec2 pA = b2TransformPoint( xfA, base->localOriginAnchorA );
	b2Vec2 pB = b2TransformPoint( xfB, base->localOriginAnchorB );
	b2Vec2 dp = b2Sub( pB, pA );

	switch ( joint->type )
	{
		case b2_distanceJoint:
		{
			b2DistanceJoint* distanceJoint = &base->distanceJoint;
			float length = b2Length( dp );
			if ( distanceJoint->enableSpring )
			{
				if ( distanceJoint->enableLimit )
				{
					if ( length < distanceJoint->minLength )
					{
						return distanceJoint->minLength - length;
					}
					else if ( length > distanceJoint->maxLength )
					{
						return length - distanceJoint->maxLength;
					}

					return 0.0f;
				}

				return 0.0f;
			}

			return b2AbsFloat( length - distanceJoint->length );
		}

		case b2_motorJoint:
			return 0.0f;

		case b2_mouseJoint:
			return 0.0f;

		case b2_filterJoint:
			return 0.0f;

		case b2_prismaticJoint:
		{
			b2PrismaticJoint* prismaticJoint = &base->prismaticJoint;
			b2Vec2 axisA = b2RotateVector( xfA.q, prismaticJoint->localAxisA );
			b2Vec2 perpA = b2LeftPerp( axisA );
			float perpendicularSeparation = b2AbsFloat( b2Dot( perpA, dp ) );
			float limitSeparation = 0.0f;

			if ( prismaticJoint->enableLimit )
			{
				float translation = b2Dot( axisA, dp );
				if ( translation < prismaticJoint->lowerTranslation )
				{
					limitSeparation = prismaticJoint->lowerTranslation - translation;
				}

				if ( prismaticJoint->upperTranslation < translation )
				{
					limitSeparation = translation - prismaticJoint->upperTranslation;
				}
			}

			return sqrtf( perpendicularSeparation * perpendicularSeparation + limitSeparation * limitSeparation );
		}

		case b2_revoluteJoint:
			return b2Length( dp );

		case b2_weldJoint:
		{
			b2WeldJoint* weldJoint = &base->weldJoint;
			if ( weldJoint->linearHertz == 0.0f )
			{
				return b2Length( dp );
			}

			return 0.0f;
		}

		case b2_wheelJoint:
		{
			b2WheelJoint* wheelJoint = &base->wheelJoint;
			b2Vec2 axisA = b2RotateVector( xfA.q, wheelJoint->localAxisA );
			b2Vec2 perpA = b2LeftPerp( axisA );
			float perpendicularSeparation = b2AbsFloat( b2Dot( perpA, dp ) );
			float limitSeparation = 0.0f;

			if ( wheelJoint->enableLimit )
			{
				float translation = b2Dot( axisA, dp );
				if ( translation < wheelJoint->lowerTranslation )
				{
					limitSeparation = wheelJoint->lowerTranslation - translation;
				}

				if ( wheelJoint->upperTranslation < translation )
				{
					limitSeparation = translation - wheelJoint->upperTranslation;
				}
			}

			return sqrtf( perpendicularSeparation * perpendicularSeparation + limitSeparation * limitSeparation );
		}

		default:
			B2_ASSERT( false );
			return 0.0f;
	}
}

float b2Joint_GetAngularSeparation( b2JointId jointId )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* base = b2GetJointSim( world, joint );

	b2Transform xfA = b2GetBodyTransform( world, joint->edges[0].bodyId );
	b2Transform xfB = b2GetBodyTransform( world, joint->edges[1].bodyId );
	float relativeAngle = b2RelativeAngle( xfB.q, xfA.q );

	switch ( joint->type )
	{
		case b2_distanceJoint:
			return 0.0f;

		case b2_motorJoint:
			return 0.0f;

		case b2_mouseJoint:
			return 0.0f;

		case b2_filterJoint:
			return 0.0f;

		case b2_prismaticJoint:
		{
			b2PrismaticJoint* prismaticJoint = &base->prismaticJoint;
			return b2UnwindAngle( relativeAngle - prismaticJoint->referenceAngle );
		}

		case b2_revoluteJoint:
		{
			b2RevoluteJoint* revoluteJoint = &base->revoluteJoint;
			if ( revoluteJoint->enableLimit )
			{
				float angle = b2UnwindAngle( relativeAngle - revoluteJoint->referenceAngle );
				if ( angle < revoluteJoint->lowerAngle )
				{
					return revoluteJoint->lowerAngle - angle;
				}

				if ( revoluteJoint->upperAngle < angle )
				{
					return angle - revoluteJoint->upperAngle;
				}
			}

			return 0.0f;
		}

		case b2_weldJoint:
		{
			b2WeldJoint* weldJoint = &base->weldJoint;
			if ( weldJoint->angularHertz == 0.0f )
			{
				return b2UnwindAngle( relativeAngle - weldJoint->referenceAngle );
			}

			return 0.0f;
		}

		case b2_wheelJoint:
			return 0.0f;

		default:
			B2_ASSERT( false );
			return 0.0f;
	}
}

void b2Joint_GetConstraintTuning( b2JointId jointId, float* hertz, float* dampingRatio )
{
	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* base = b2GetJointSim( world, joint );
	*hertz = base->constraintHertz;
	*dampingRatio = base->constraintDampingRatio;
}

void b2Joint_SetConstraintTuning( b2JointId jointId, float hertz, float dampingRatio )
{
	B2_ASSERT( b2IsValidFloat( hertz ) && hertz >= 0.0f );
	B2_ASSERT( b2IsValidFloat( dampingRatio ) && dampingRatio >= 0.0f );

	b2World* world = b2GetWorld( jointId.world0 );
	b2Joint* joint = b2GetJointFullId( world, jointId );
	b2JointSim* base = b2GetJointSim( world, joint );
	base->constraintHertz = hertz;
	base->constraintDampingRatio = dampingRatio;
}

void b2PrepareJoint( b2JointSim* joint, b2StepContext* context )
{
	// Clamp joint hertz based on the time step to reduce jitter.
	float hertz = b2MinFloat( joint->constraintHertz, 0.25f * context->inv_h );
	joint->constraintSoftness = b2MakeSoft( hertz, joint->constraintDampingRatio, context->h );

	switch ( joint->type )
	{
		case b2_distanceJoint:
			b2PrepareDistanceJoint( joint, context );
			break;

		case b2_motorJoint:
			b2PrepareMotorJoint( joint, context );
			break;

		case b2_mouseJoint:
			b2PrepareMouseJoint( joint, context );
			break;

		case b2_filterJoint:
			break;

		case b2_prismaticJoint:
			b2PreparePrismaticJoint( joint, context );
			break;

		case b2_revoluteJoint:
			b2PrepareRevoluteJoint( joint, context );
			break;

		case b2_weldJoint:
			b2PrepareWeldJoint( joint, context );
			break;

		case b2_wheelJoint:
			b2PrepareWheelJoint( joint, context );
			break;

		default:
			B2_ASSERT( false );
	}
}

void b2WarmStartJoint( b2JointSim* joint, b2StepContext* context )
{
	switch ( joint->type )
	{
		case b2_distanceJoint:
			b2WarmStartDistanceJoint( joint, context );
			break;

		case b2_motorJoint:
			b2WarmStartMotorJoint( joint, context );
			break;

		case b2_mouseJoint:
			b2WarmStartMouseJoint( joint, context );
			break;

		case b2_filterJoint:
			break;

		case b2_prismaticJoint:
			b2WarmStartPrismaticJoint( joint, context );
			break;

		case b2_revoluteJoint:
			b2WarmStartRevoluteJoint( joint, context );
			break;

		case b2_weldJoint:
			b2WarmStartWeldJoint( joint, context );
			break;

		case b2_wheelJoint:
			b2WarmStartWheelJoint( joint, context );
			break;

		default:
			B2_ASSERT( false );
	}
}

void b2SolveJoint( b2JointSim* joint, b2StepContext* context, bool useBias )
{
	switch ( joint->type )
	{
		case b2_distanceJoint:
			b2SolveDistanceJoint( joint, context, useBias );
			break;

		case b2_motorJoint:
			b2SolveMotorJoint( joint, context, useBias );
			break;

		case b2_mouseJoint:
			b2SolveMouseJoint( joint, context );
			break;

		case b2_filterJoint:
			break;

		case b2_prismaticJoint:
			b2SolvePrismaticJoint( joint, context, useBias );
			break;

		case b2_revoluteJoint:
			b2SolveRevoluteJoint( joint, context, useBias );
			break;

		case b2_weldJoint:
			b2SolveWeldJoint( joint, context, useBias );
			break;

		case b2_wheelJoint:
			b2SolveWheelJoint( joint, context, useBias );
			break;

		default:
			B2_ASSERT( false );
	}
}

void b2PrepareOverflowJoints( b2StepContext* context )
{
	b2TracyCZoneNC( prepare_joints, "PrepJoints", b2_colorOldLace, true );

	b2ConstraintGraph* graph = context->graph;
	b2JointSim* joints = graph->colors[B2_OVERFLOW_INDEX].jointSims.data;
	int jointCount = graph->colors[B2_OVERFLOW_INDEX].jointSims.count;

	for ( int i = 0; i < jointCount; ++i )
	{
		b2JointSim* joint = joints + i;
		b2PrepareJoint( joint, context );
	}

	b2TracyCZoneEnd( prepare_joints );
}

void b2WarmStartOverflowJoints( b2StepContext* context )
{
	b2TracyCZoneNC( prepare_joints, "PrepJoints", b2_colorOldLace, true );

	b2ConstraintGraph* graph = context->graph;
	b2JointSim* joints = graph->colors[B2_OVERFLOW_INDEX].jointSims.data;
	int jointCount = graph->colors[B2_OVERFLOW_INDEX].jointSims.count;

	for ( int i = 0; i < jointCount; ++i )
	{
		b2JointSim* joint = joints + i;
		b2WarmStartJoint( joint, context );
	}

	b2TracyCZoneEnd( prepare_joints );
}

void b2SolveOverflowJoints( b2StepContext* context, bool useBias )
{
	b2TracyCZoneNC( solve_joints, "SolveJoints", b2_colorLemonChiffon, true );

	b2ConstraintGraph* graph = context->graph;
	b2JointSim* joints = graph->colors[B2_OVERFLOW_INDEX].jointSims.data;
	int jointCount = graph->colors[B2_OVERFLOW_INDEX].jointSims.count;

	for ( int i = 0; i < jointCount; ++i )
	{
		b2JointSim* joint = joints + i;
		b2SolveJoint( joint, context, useBias );
	}

	b2TracyCZoneEnd( solve_joints );
}

void b2DrawJoint( b2DebugDraw* draw, b2World* world, b2Joint* joint )
{
	b2Body* bodyA = b2BodyArray_Get( &world->bodies, joint->edges[0].bodyId );
	b2Body* bodyB = b2BodyArray_Get( &world->bodies, joint->edges[1].bodyId );
	if ( bodyA->setIndex == b2_disabledSet || bodyB->setIndex == b2_disabledSet )
	{
		return;
	}

	b2JointSim* jointSim = b2GetJointSim( world, joint );

	b2Transform transformA = b2GetBodyTransformQuick( world, bodyA );
	b2Transform transformB = b2GetBodyTransformQuick( world, bodyB );
	b2Vec2 pA = b2TransformPoint( transformA, jointSim->localOriginAnchorA );
	b2Vec2 pB = b2TransformPoint( transformB, jointSim->localOriginAnchorB );

	b2HexColor color = b2_colorDarkSeaGreen;

	switch ( joint->type )
	{
		case b2_distanceJoint:
			b2DrawDistanceJoint( draw, jointSim, transformA, transformB );
			break;

		case b2_mouseJoint:
		{
			b2Vec2 target = jointSim->mouseJoint.targetA;

			b2HexColor c1 = b2_colorGreen;
			draw->DrawPointFcn( target, 4.0f, c1, draw->context );
			draw->DrawPointFcn( pB, 4.0f, c1, draw->context );

			b2HexColor c2 = b2_colorLightGray;
			draw->DrawSegmentFcn( target, pB, c2, draw->context );
		}
		break;

		case b2_filterJoint:
		{
			draw->DrawSegmentFcn( pA, pB, b2_colorGold, draw->context );
		}
		break;

		case b2_prismaticJoint:
			b2DrawPrismaticJoint( draw, jointSim, transformA, transformB );
			break;

		case b2_revoluteJoint:
			b2DrawRevoluteJoint( draw, jointSim, transformA, transformB, joint->drawSize );
			break;

		case b2_wheelJoint:
			b2DrawWheelJoint( draw, jointSim, transformA, transformB );
			break;

		default:
			draw->DrawSegmentFcn( transformA.p, pA, color, draw->context );
			draw->DrawSegmentFcn( pA, pB, color, draw->context );
			draw->DrawSegmentFcn( transformB.p, pB, color, draw->context );
	}

	if ( draw->drawGraphColors )
	{
		b2HexColor colors[B2_GRAPH_COLOR_COUNT] = { b2_colorRed,	   b2_colorOrange,	  b2_colorYellow, b2_colorGreen,
													b2_colorCyan,	   b2_colorBlue,	  b2_colorViolet, b2_colorPink,
													b2_colorChocolate, b2_colorGoldenRod, b2_colorCoral,  b2_colorBlack };

		int colorIndex = joint->colorIndex;
		if ( colorIndex != B2_NULL_INDEX )
		{
			b2Vec2 p = b2Lerp( pA, pB, 0.5f );
			draw->DrawPointFcn( p, 5.0f, colors[colorIndex], draw->context );
		}
	}
}
