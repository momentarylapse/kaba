//
// Created by Michael Ankele on 2024-06-23.
//

#ifndef BACKENDARM64_H
#define BACKENDARM64_H

#include "BackendARM.h"

namespace kaba {

class BackendArm64 : public BackendARM {
public:
	explicit BackendArm64(Serializer* serializer);

};

} // kaba

#endif //BACKENDARM64_H
