#ifndef SPECIESRRLL_H
#define SPECIESRRLL_H

#include "Species.h"

class ElectroMagn;
class Pusher;
class Interpolator;
class Projector;
class Params;

//! class Species_rrll
//! Species for which the dynamics is governed by the Lorentz force and the classical radiation reaction force:
//! Boris pusher + first order spliting
class Species_rrll : public Species
{
public:
    //! Creator for Species_rrLL
    Species_rrll(Params&, Patch*);
    //! Destructor for Species_rrLL
    ~Species_rrll();

private:

};

#endif
