#pragma once

#include "btBulletDynamicsCommon.h"
#include <stdio.h>


class PhysicsWorld
{

public:
	
	btDiscreteDynamicsWorld* dynamicsWorld;
	
	
	PhysicsWorld() {
		///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		collisionConfiguration = new btDefaultCollisionConfiguration();

		///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		dispatcher = new btCollisionDispatcher(collisionConfiguration);

		///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		overlappingPairCache = new btDbvtBroadphase();

		///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		solver = new btSequentialImpulseConstraintSolver;

		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0, -10, 0));

	}

	

	~PhysicsWorld() {

		int i;

		//remove the rigidbodies from the dynamics world and delete them
		for (i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}
			dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}

		//delete collision shapes
		for (int j = 0; j < collisionShapes.size(); j++)
		{
			btCollisionShape* shape = collisionShapes[j];
			collisionShapes[j] = 0;
			delete shape;
		}

		//delete dynamics world
		delete dynamicsWorld;

		//delete solver
		delete solver;

		//delete broadphase
		delete overlappingPairCache;

		//delete dispatcher
		delete dispatcher;

		delete collisionConfiguration;

		//next line is optional: it will be cleared by the destructor when the array goes out of scope
		collisionShapes.clear();
	}

	void Init()
	{
		groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(.1), btScalar(50.)));
		//keep track of the shapes, we release memory at exit.
		//make sure to re-use collision shapes among rigid bodies whenever possible!
		///create a few basic rigid bodies


		//the ground is a cube of side 100 at position y = -56.
		//the sphere will hit it at y = -6, with center at -5

		collisionShapes.push_back(groundShape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -5, 0));

		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);

	}

	void CreateStaticBoxBody(float x, float y, float z, float size) {

		size /= 2;

		//create a dynamic rigidbody

		btCollisionShape* colShape = new btBoxShape(btVector3(size, size, size));

		collisionShapes.push_back(colShape);

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar mass(0.0f);
		btVector3 localInertia(0, 0, 0);


		startTransform.setOrigin(btVector3(x, y, z));

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);

	}



	void CreateBoxBody(float x, float y, float z, float size, float xf, float yf, float zf, float force) {

		size /= 2;

		//create a dynamic rigidbody

		btCollisionShape* colShape = new btBoxShape(btVector3(size,size,size));

		collisionShapes.push_back(colShape);

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar mass(1.f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		startTransform.setOrigin(btVector3(x, y, z));

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);
		body->applyImpulse(btVector3(xf, yf, zf) * force, btVector3(0.2,0,0));
		body->applyCentralImpulse(btVector3(xf, yf, zf)*force);
		
	}

	void Tick(float deltaTime) {
		dynamicsWorld->stepSimulation(deltaTime,10);
	}

	btDiscreteDynamicsWorld* GetDynamics() {
		return dynamicsWorld;
	}


private:
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;
	btCollisionShape* groundShape;
};

