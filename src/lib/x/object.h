/*----------------------------------------------------------------------------*\
| CObject                                                                      |
| -> physical entities of a model in the game                                  |
| -> manages physics on its own                                                |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2009.12.03 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(OBJECT_H__INCLUDED_)
#define OBJECT_H__INCLUDED_



class CObject : public CModel
{
public:
//	CObject(const char *filename, const char *name, const vector &pos);
//	CObject(CModel *model);
	CObject();
//	~CObject();
	void UpdateMatrix();

	void UpdateData(); // script...

	void UpdateTheta();
//	void SetMaterial(sMaterial *material, int mode);
//	void ObjectCalcMove();
	void DoPhysics();
//	CObject *CuttingPlane(plane pl);
//	void CorrectVel(CObject *partner);

	void AddForce(const vector &f, const vector &rho);
	void AddTorque(const vector &t);

	void MakeVisible(bool visible);
};


#endif

