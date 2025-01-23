#pragma once

#include "vec3.h"

struct Box {
	vec3 min, max;

	vec3 center() const;
	vec3 size() const;
	Box canonical() const;
	vec3 to_relative(const vec3& p) const;
	vec3 to_absolute(const vec3& p) const;
};

