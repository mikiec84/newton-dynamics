/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgBallConstraint.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dgBallConstraint::dgBallConstraint ()
	:dgBilateralConstraint() 
{
	
//	dgBallConstraint* constraint;

//	dgBallConstraintArray& array = * world;
//	constraint = array.GetElement();
	dgAssert ((((dgUnsigned64) &m_localMatrix0) & 15) == 0);
	
	m_localMatrix0 = dgGetIdentityMatrix();
	m_localMatrix1 = dgGetIdentityMatrix();

	//constraint->SetStiffness (dgFloat32 (0.5f));
	m_maxDOF = 6;
	m_jointUserCallback = NULL;
	m_constId = m_ballConstraint;
	m_ballLimits = 0;
	m_angles = dgVector (dgFloat32 (0.0f));
}

dgBallConstraint::~dgBallConstraint ()
{
}

/*
dgBallConstraint* dgBallConstraint::Create(dgWorld* world)
{
	dgBallConstraint* constraint;


	dgBallConstraintArray& array = * world;
	constraint = array.GetElement();
	dgAssert ((((dgUnsigned64) &constraint->m_localMatrix0) & 15) == 0);

	constraint->Init();

//	constraint->SetStiffness (dgFloat32 (0.5f));
	constraint->m_maxDOF = 6;
	constraint->m_jointUserCallback = NULL;
	constraint->m_constId = dgBallConstraintId;
	constraint->m_ballLimits = 0;
	constraint->m_angles = dgVector (dgFloat32 (0.0f));
	return constraint;
}


void dgBallConstraint::Remove(dgWorld* world)
{
	dgBallConstraintArray& array = *world;
	dgBilateralConstraint::Remove (world);
	array.RemoveElement (this);
}
*/

void dgBallConstraint::SetJointParameterCallback (dgBallJointFriction callback)
{
	m_jointUserCallback = callback;
}


dgVector dgBallConstraint::GetJointAngle ()const
{
	return m_angles;
}

dgVector dgBallConstraint::GetJointOmega () const
{
	dgAssert (m_body0);
	dgAssert (m_body1);
	const dgMatrix& matrix = m_body0->GetMatrix();

	dgVector dir0 (matrix.RotateVector (m_localMatrix0[0]));
	dgVector dir1 (matrix.RotateVector (m_localMatrix0[1]));
	dgVector dir2 (matrix.RotateVector (m_localMatrix0[2]));

	const dgVector& omega0 = m_body0->GetOmega();
	const dgVector& omega1 = m_body1->GetOmega();

//	dgVector omega1 (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
//	if (m_body1) {
//		omega1 = m_body1->GetOmega();
//	}

	dgVector relOmega (omega0 - omega1);
	return dgVector (relOmega.DotProduct(dir0).GetScalar(), relOmega.DotProduct(dir1).GetScalar(), relOmega.DotProduct(dir2).GetScalar(), dgFloat32 (0.0f));
}

dgVector dgBallConstraint::GetJointForce () const
{
	dgMatrix matrix0;
	dgMatrix matrix1;

	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	return dgVector (matrix0.m_front.Scale (m_jointForce[0].m_force) + matrix0.m_up.Scale (m_jointForce[1].m_force) + matrix0.m_right.Scale (m_jointForce[2].m_force));
}

bool dgBallConstraint::GetTwistLimitState () const
{
	return m_twistLimit;
}

bool dgBallConstraint::GetConeLimitState () const
{
	return m_coneLimit;	
}

bool dgBallConstraint::GetLatealLimitState () const
{
	return m_lateralLimit;	
}

void dgBallConstraint::SetTwistLimitState (bool state)
{
	m_twistLimit = dgUnsigned32 (state);
}

void dgBallConstraint::SetConeLimitState (bool state)
{
	m_coneLimit = dgUnsigned32 (state);	
}

void dgBallConstraint::SetLatealLimitState (bool state)
{
	m_lateralLimit = dgUnsigned32 (state);	
}

void dgBallConstraint::SetPivotPoint(const dgVector &pivot)
{
	dgAssert (m_body0);
	dgAssert (m_body1);
	const dgMatrix& matrix = m_body0->GetMatrix();

	dgVector pin (pivot - matrix.m_posit); 
	dgAssert (pin.m_w == dgFloat32 (0.0f));
	if (pin.DotProduct(pin).GetScalar() < dgFloat32 (1.0e-3f)) {
		pin = matrix.m_front;
	}

	SetPivotAndPinDir (pivot, pin, m_localMatrix0, m_localMatrix1);

	dgMatrix matrix0;
	dgMatrix matrix1;
	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);
	SetLimits (matrix0.m_front, -dgPi * dgFloat32 (0.5f), dgPi * dgFloat32 (0.5f), dgPi * dgFloat32 (0.5f), matrix0.m_right, dgFloat32 (0.0f), dgFloat32 (0.0f));
}

void dgBallConstraint::SetLimits (
	const dgVector& coneDir, 
	dgFloat32 minConeAngle, 
	dgFloat32 maxConeAngle,
	dgFloat32 maxTwistAngle,
	const dgVector& bilateralDir, 
	dgFloat32 negativeBilateralConeAngle__,
	dgFloat32 positiveBilateralConeAngle__) 
{
	dgMatrix matrix0;
	dgMatrix matrix1;
	CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1);

	dgAssert (m_body0);
	dgAssert (m_body1);
	const dgMatrix& body0_Matrix = m_body0->GetMatrix();

	dgVector lateralDir (bilateralDir.CrossProduct(coneDir));
	if (lateralDir.DotProduct(lateralDir).GetScalar() < dgFloat32 (1.0e-3f)) {
		dgMatrix tmp (coneDir);
		lateralDir = tmp.m_up;
	}
	

	m_localMatrix0.m_front = body0_Matrix.UnrotateVector (coneDir);
	m_localMatrix0.m_up = body0_Matrix.UnrotateVector (lateralDir);
	m_localMatrix0.m_posit = body0_Matrix.UntransformVector (matrix1.m_posit);

	m_localMatrix0.m_front = m_localMatrix0.m_front.Normalize();
	m_localMatrix0.m_up = m_localMatrix0.m_up.Normalize();
	m_localMatrix0.m_right = m_localMatrix0.m_front.CrossProduct(m_localMatrix0.m_up);

	m_localMatrix0.m_front.m_w = dgFloat32 (0.0f);
	m_localMatrix0.m_up.m_w    = dgFloat32 (0.0f);
	m_localMatrix0.m_right.m_w = dgFloat32 (0.0f);
	m_localMatrix0.m_posit.m_w = dgFloat32 (1.0f);

	const dgMatrix& body1_Matrix = m_body1->GetMatrix();

	m_twistAngle = dgClamp (maxTwistAngle, dgFloat32 (5.0f) * dgDegreeToRad, dgFloat32 (90.0f) * dgDegreeToRad);
	m_coneAngle = dgClamp ((maxConeAngle - minConeAngle) * dgFloat32 (0.5f), dgFloat32 (5.0f) * dgDegreeToRad, 175.0f * dgDegreeToRad);
	m_coneAngleCos = dgCos (m_coneAngle);

	dgMatrix coneMatrix (dgPitchMatrix((maxConeAngle + minConeAngle) * dgFloat32 (0.5f)));

	m_localMatrix0 = coneMatrix * m_localMatrix0;

	m_localMatrix1 = m_localMatrix0 * body0_Matrix * body1_Matrix.Inverse();

}

dgUnsigned32 dgBallConstraint::JacobianDerivative (dgContraintDescritor& params)
{
	dgMatrix matrix0;
	dgMatrix matrix1;

	if (m_jointUserCallback) {
		m_jointUserCallback (*this, params.m_timestep);
	}

	dgVector angle (CalculateGlobalMatrixAndAngle (m_localMatrix0, m_localMatrix1, matrix0, matrix1));
	m_angles = angle.Scale (-dgFloat32 (1.0f));

	const dgVector& dir0 = matrix0.m_front;
	const dgVector& dir1 = matrix0.m_up;
	const dgVector& dir2 = matrix0.m_right;
	const dgVector& p0 = matrix0.m_posit;
	const dgVector& p1 = matrix1.m_posit;


	dgPointParam pointData;
    InitPointParam (pointData, m_stiffness, p0, p1);
	CalculatePointDerivative (0, params, dir0, pointData, &m_jointForce[0]); 
	CalculatePointDerivative (1, params, dir1, pointData, &m_jointForce[1]); 
	CalculatePointDerivative (2, params, dir2, pointData, &m_jointForce[2]); 
	dgInt32 ret = 3;

	dgAssert (0);
/*
	dgFloat32 relVelocErr;
	dgFloat32 penetrationErr;
	if (m_twistLimit) {
		if (angle.m_x > m_twistAngle) {
			dgVector q0 (matrix0.m_posit + matrix0.m_up.Scale(MIN_JOINT_PIN_LENGTH));
			InitPointParam (pointData, m_stiffness, q0, q0);

			const dgVector& dir = matrix0.m_right;
			CalculatePointDerivative (ret, params, dir, pointData, &m_jointForce[ret]); 

			dgVector velocError (pointData.m_veloc1 - pointData.m_veloc0);
			relVelocErr = velocError.DotProduct(dir).GetScalar();
			if (relVelocErr > dgFloat32 (1.0e-3f)) {
				relVelocErr *= dgFloat32 (1.1f);
			}

			penetrationErr = MIN_JOINT_PIN_LENGTH * (angle.m_x - m_twistAngle); 
			dgAssert (penetrationErr >= dgFloat32 (0.0f));
		
			params.m_forceBounds[ret].m_low = dgFloat32 (0.0f);
			params.m_forceBounds[ret].m_normalIndex = DG_INDEPENDENT_ROW;
			params.m_forceBounds[ret].m_jointForce = &m_jointForce[ret];
			SetMotorAcceleration (ret, (relVelocErr + penetrationErr) * params.m_invTimestep, params);
			ret ++;
		} else if (angle.m_x < - m_twistAngle) {
			dgVector q0 (matrix0.m_posit + matrix0.m_up.Scale(MIN_JOINT_PIN_LENGTH));
			InitPointParam (pointData, m_stiffness, q0, q0);
			//dgVector dir (matrix0.m_right.Scale (-dgFloat32 (1.0f)));
			dgVector dir (matrix0.m_right * dgVector::m_negOne);
			CalculatePointDerivative (ret, params, dir, pointData, &m_jointForce[ret]); 

			dgVector velocError (pointData.m_veloc1 - pointData.m_veloc0);
			relVelocErr = velocError.DotProduct(dir).GetScalar();
			if (relVelocErr > dgFloat32 (1.0e-3f)) {
				relVelocErr *= dgFloat32 (1.1f);
			}

			penetrationErr = MIN_JOINT_PIN_LENGTH * (- m_twistAngle - angle.m_x); 
			dgAssert (penetrationErr >= dgFloat32 (0.0f));
		
			params.m_forceBounds[ret].m_low = dgFloat32 (0.0f);
			params.m_forceBounds[ret].m_normalIndex = DG_INDEPENDENT_ROW;
			params.m_forceBounds[ret].m_jointForce = &m_jointForce[ret];
			SetMotorAcceleration (ret, (relVelocErr + penetrationErr) * params.m_invTimestep, params);
			ret ++;
		}
	}

	if (m_coneLimit) {

		dgFloat32 coneCos;
		coneCos = matrix0.m_front.DotProduct(matrix1.m_front).GetScalar();
		if (coneCos < m_coneAngleCos) {
			dgVector q0 (matrix0.m_posit + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
			InitPointParam (pointData, m_stiffness, q0, q0);

			dgVector tangentDir (matrix0.m_front.CrossProduct(matrix1.m_front));
			tangentDir = tangentDir.Normalize());
			CalculatePointDerivative (ret, params, tangentDir, pointData, &m_jointForce[ret]); 
			ret ++;

			dgVector normalDir (tangentDir.CrossProduct(matrix0.m_front));

			dgVector velocError (pointData.m_veloc1 - pointData.m_veloc0);
			//restitution = contact.m_restitution;
			relVelocErr = velocError.DotProduct(normalDir).GetScalar();
			if (relVelocErr > dgFloat32 (1.0e-3f)) {
				relVelocErr *= dgFloat32 (1.1f);
			}

			penetrationErr = MIN_JOINT_PIN_LENGTH * (dgAcos (dgMax (coneCos, dgFloat32(-0.9999f))) - m_coneAngle); 
			dgAssert (penetrationErr >= dgFloat32 (0.0f));

			CalculatePointDerivative (ret, params, normalDir, pointData, &m_jointForce[ret]); 
			params.m_forceBounds[ret].m_low = dgFloat32 (0.0f);
			params.m_forceBounds[ret].m_normalIndex = DG_INDEPENDENT_ROW;
			params.m_forceBounds[ret].m_jointForce = &m_jointForce[ret];
			SetMotorAcceleration (ret, (relVelocErr + penetrationErr) * params.m_invTimestep, params);
			ret ++;
		}
	}
*/
	return dgUnsigned32 (ret);
}

