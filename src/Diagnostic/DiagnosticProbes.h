#ifndef DIAGNOSTICPROBES_H
#define DIAGNOSTICPROBES_H

#include <hdf5.h>

#include "Diagnostic.h"

#include "Params.h"
#include "Patch.h"
#include "SmileiMPI.h"

#include "Field2D.h"


class DiagnosticProbes : public Diagnostic {
    friend class SmileiMPI;

public :
    
    //! Default constructor
    DiagnosticProbes( Params &params, SmileiMPI* smpi, int n_probe );
    //! Default destructor
    ~DiagnosticProbes() override;
    
    void openFile( Params& params, SmileiMPI* smpi, bool newfile ) override;
    
    void closeFile() override;
    
    bool prepare( int timestep ) override;
    
    void run( SmileiMPI* smpi, VectorPatch& vecPatches, int timestep ) override;
    
    void init(Params& params, SmileiMPI* smpi, VectorPatch& vecPatches) override;
    
private :
    
    //! Index of the probe diagnostic
    int probe_n;
    
    //! Dimension of the probe grid
    unsigned int dimProbe;
    
    //! Dimension of particle coordinates
    unsigned int nDim_particle;
    
    //! Number of points in each dimension
    std::vector<unsigned int> vecNumber; 
    
    //! List of the coordinates of the probe vertices
    std::vector< std::vector<double> > allPos;
    
    //! number of fake particles for each probe diagnostic
    unsigned int nPart_total;
    
    //! Number of fields to save
    int nFields;
    
    //! List of fields to save
    std::vector<std::string> fieldname;
    
    //! Indices in the output array where each field goes
    std::vector<unsigned int> fieldlocation;
    
    //! E local fields for the projector
    LocalFields Eloc_fields;
    //! B local fields for the projector
    LocalFields Bloc_fields;
    //! J local fields for the projector
    LocalFields Jloc_fields;
    //! Rho local field for the projector
    double Rloc_fields;
    
};



class ProbeParticles {
public :
    ProbeParticles() {};
    ProbeParticles( ProbeParticles* probe ) { offset_in_file=probe->offset_in_file; }
    ~ProbeParticles() {};
    
    Particles particles;
    int offset_in_file;
};





#endif

