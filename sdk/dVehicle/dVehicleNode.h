/* Copyright (c) <2003-2019> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


#ifndef __D_VEHICLE_NODE_H__
#define __D_VEHICLE_NODE_H__

#include "dStdafxVehicle.h"

class dVehicleNode;
class dVehicleTire;
class dVehicleChassis;
class dVehicleLoopJoint;

class dVehicleNodeChildrenList: public dList<dVehicleNode*>
{
	public:
	dVehicleNodeChildrenList()
		:dList<dVehicleNode*>()
	{
	}

	~dVehicleNodeChildrenList()
	{
	}
};

class dVehicleNode: public dCustomAlloc
{
	public:
	DVEHICLE_API dVehicleNode(dVehicleNode* const parent);
	DVEHICLE_API virtual ~dVehicleNode();
	
	DVEHICLE_API virtual void CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const;
	
	virtual dVehicleTire* GetAsTire() const { return NULL; }
	virtual dVehicleChassis* GetAsVehicle() const { return NULL; }
	virtual dComplementaritySolver::dBilateralJoint* GetJoint() { return NULL; }
		
	void* GetUserData() const {return m_usedData;}
	void SetUserData(void* const userData) {m_usedData = userData;}

	dComplementaritySolver::dBodyState& GetProxyBody() { return m_proxyBody; }

	protected:
	//virtual void RigidBodyToStates();
	virtual void ApplyExternalForce();
	//virtual void Integrate(dFloat timestep);
	//virtual void StatesToRigidBody(dFloat timestep);

	virtual int GetKinematicLoops(dVehicleLoopJoint** const jointArray);
	void CalculateAABB(const NewtonCollision* const collision, const dMatrix& matrix, dVector& minP, dVector& maxP) const;
	

	
	virtual const void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	dComplementaritySolver::dBodyState m_proxyBody;
	void* m_usedData;
	dVehicleNode* m_parent;
	dVehicleNodeChildrenList m_children;
	int m_index;

	friend class dVehicleSolver;
	friend class dVehicleManager;
	friend class dVehicleChassis;
};


#endif 

