
#ifndef RANDOM_H_
#define RANDOM_H_

class Random
{
public:
	Random();
	void _cdecl __init__();
	void _cdecl __assign__(Random *other);

	void _cdecl seed(const string &s);
	int _cdecl _get();
	int _cdecl _int(int max);
	float _cdecl uniform01();
	float _cdecl uniform(float min, float max);
	float _cdecl normal(float mean, float stddev);

	vector _cdecl in_ball(float r);
	vector _cdecl dir();

private:
	int Q[4096];
	int c;
};


#endif
