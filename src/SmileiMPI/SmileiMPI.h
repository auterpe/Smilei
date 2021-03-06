#ifndef SMILEIMPI_H
#define SMILEIMPI_H

#include <string>
#include <vector>

#include <mpi.h>

#include "Params.h"
#include "Tools.h"
#include "Particles.h"
#include "Field.h"

class Params;
class Species;
class VectorPatch;

class ElectroMagn;
class Field;
class ProbeParticles;

class Diagnostic;
class DiagnosticScalar;
class DiagnosticParticles;

#define SMILEI_COMM_DUMP_TIME 1312

//  --------------------------------------------------------------------------------------------------------------------
//! Class SmileiMPI
//  --------------------------------------------------------------------------------------------------------------------
class SmileiMPI {
    friend class Checkpoint;
    friend class PatchesFactory;
    friend class Patch;
    friend class VectorPatch;

public:

    //! Create intial MPI environment
    SmileiMPI( int* argc, char*** argv );
    //! Destructor for SmileiMPI
    ~SmileiMPI();

    // Broadcast a string in current communicator
    void bcast( std::string& val );
    // Broadcast an int in current communicator
    void bcast( int& val );

    //! Initialize  MPI (per process) environment
    //! \param params Parameters
    void init( Params& params );

    // Initialize the patch_count vector. Patches are distributed in order to balance the load between MPI processes.
    void init_patch_count( Params& params );
    // Recompute the patch_count vector. Browse patches and redistribute them in order to balance the load between MPI processes.
    void recompute_patch_count( Params& params, VectorPatch& vecpatches, double time_dual );
     // Returns the rank of the MPI process currently owning patch h.
    int hrank(int h);

    // Create MPI type to exchange all particles properties of particles
    MPI_Datatype createMPIparticles( Particles* particles );


    // PATCH SEND / RECV METHODS
    //     - during load balancing process
    //     - during moving window
    // -----------------------------------
    void isend(Patch* patch, int to  , int hindex, Params& params);
    void recv (Patch* patch, int from, int hindex, Params& params);

    void isend(Particles* particles, int to   , int hindex, MPI_Datatype datatype);
    void recv (Particles* partictles, int from, int hindex, MPI_Datatype datatype);
    void isend(std::vector<int>* vec, int to  , int hindex);
    void recv (std::vector<int> *vec, int from, int hindex);

    void isend(ElectroMagn* fields, int to  , int hindex);
    void recv (ElectroMagn* fields, int from, int hindex);
    void isend(Field* field, int to  , int hindex);
    void recv (Field* field, int from, int hindex);
    void isend( ProbeParticles* probe, int to  , int hindex, unsigned int );
    void recv ( ProbeParticles* probe, int from, int hindex, unsigned int );


    // DIAGS MPI SYNC 
    // --------------

    // Wrapper of MPI synchronization of all computing diags
    void computeGlobalDiags(Diagnostic*          diag, int timestep);
    // MPI synchronization of scalars diags
    void computeGlobalDiags(DiagnosticScalar*    diag, int timestep);
    // MPI synchronization of diags particles
    void computeGlobalDiags(DiagnosticParticles* diag, int timestep);
        

    // MPI basic methods
    // -----------------

    //! Method to identify the rank 0 MPI process
    inline bool isMaster() {
        return (smilei_rk==0);
    }
    //! Method to synchronize MPI process in the current MPI communicator
    inline void barrier() {
        MPI_Barrier( SMILEI_COMM_WORLD );
    }
    //! Return MPI_Comm_rank
    inline int getRank() {
        return smilei_rk;
    }
    //! Return MPI_Comm_size
    inline int getSize() {
        return smilei_sz;
    }
    

    // Global buffers for vectorization of Species::dynamics
    // -----------------------------------------------------
    
    //! value of the Efield 
    std::vector<std::vector<LocalFields>> dynamics_Epart;
    //! value of the Bfield
    std::vector<std::vector<LocalFields>> dynamics_Bpart;
    //! gamma factor
    std::vector<std::vector<double>> dynamics_gf;
    //! iold_pos
    std::vector<std::vector<int>> dynamics_iold;
    //! delta_old_pos
    std::vector<std::vector<double>> dynamics_deltaold;

    // Resize buffers for a given number of particles
    inline void dynamics_resize(int ithread, int ndim_part, int npart ){
        dynamics_Epart[ithread].resize(npart);
        dynamics_Bpart[ithread].resize(npart);
        dynamics_gf[ithread].resize(npart);
        dynamics_iold[ithread].resize(ndim_part*npart);
        dynamics_deltaold[ithread].resize(ndim_part*npart);
    }


    // Compute global number of particles
    //     - deprecated with patch introduction
     //! \todo{Patch managmen}
    inline int globalNbrParticles(Species* species, int locNbrParticles) {
	int nParticles(0);
	MPI_Reduce( &locNbrParticles, &nParticles, 1, MPI_INT, MPI_SUM, 0, SMILEI_COMM_WORLD );
	return nParticles;
    }


protected:
    //! Global MPI Communicator
    MPI_Comm SMILEI_COMM_WORLD;

    //! Number of MPI process in the current communicator
    int smilei_sz;
    //! MPI process Id in the current communicator
    int smilei_rk;

    // Store periodicity (0/1) per direction
    // Should move in Params : last parameters of this type in this class
    int* periods_;

    //! For patch decomposition
    //Number of patches owned by each mpi process.
    std::vector<int>  patch_count, target_patch_count, capabilities;
    int Tcapabilities; //Default = smilei_sz (1 per MPI rank)


};

#endif

