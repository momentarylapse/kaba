#if !defined(LIGHT_H__INCLUDED_)
#define LIGHT_H__INCLUDED_

class vector;
class color;

namespace Light
{

	void Reset();

	int Create();
	void Delete(int index);
	void Enable(int index, bool enabled);
	void SetColors(int index, const color &am, const color &di, const color &sp);
	void SetRadial(int index, const vector &pos, float radius);
	void SetDirectional(int index, const vector &dir);

	void AddField(const vector &min, const vector &max, bool sun, const color &ambient);

	void BeginApply();
	void Apply(const vector &pos);
	void EndApply();

	vector GetSunDir();

};

#endif
