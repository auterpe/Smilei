#ifndef DIAGNOSTICFACTORY_H
#define DIAGNOSTICFACTORY_H

#include "DiagnosticParticles.h"
#include "DiagnosticProbes.h"
#include "DiagnosticScalar.h"
#include "DiagnosticTrack.h"

#include "DiagnosticFields1D.h"
#include "DiagnosticFields2D.h"

//  --------------------------------------------------------------------------------------------------------------------
//! Create appropriate IO environment for the geometry 
//! \param params : Parameters
//! \param smpi : MPI environment
//  --------------------------------------------------------------------------------------------------------------------
class DiagnosticFieldsFactory {
public:
    static Diagnostic* create(Params& params, SmileiMPI* smpi, Patch* patch, unsigned int idiag) {
        Diagnostic* diag = NULL;
        if ( params.geometry == "1d3v" ) {
            diag = new DiagnosticFields1D(params, smpi, patch, idiag);
        }
        else if ( params.geometry == "2d3v" ) {
            diag = new DiagnosticFields2D(params, smpi, patch, idiag);
        }
        else {
            ERROR( "Geometry " << params.geometry << " not implemented" );
        }
        
        return diag;
    }
};

class DiagnosticFactory {
public:

    static std::vector<Diagnostic*> createGlobalDiagnostics(Params& params, SmileiMPI* smpi, Patch* patch) {
        std::vector<Diagnostic*> vecDiagnostics;
        vecDiagnostics.push_back( new DiagnosticScalar(params, smpi, patch) );
        
        for (unsigned int n_diag_particles = 0; n_diag_particles < PyTools::nComponents("DiagParticles"); n_diag_particles++) {
            vecDiagnostics.push_back( new DiagnosticParticles(params, smpi, patch, n_diag_particles) );
        }
        
        return vecDiagnostics;
    } // END createGlobalDiagnostics
    
    
    
    
    static std::vector<Diagnostic*> createLocalDiagnostics(Params& params, SmileiMPI* smpi, Patch* patch) {
        std::vector<Diagnostic*> vecDiagnostics;
        
        for (unsigned int n_diag_fields = 0; n_diag_fields < PyTools::nComponents("DiagFields"); n_diag_fields++) {
            vecDiagnostics.push_back( DiagnosticFieldsFactory::create(params, smpi, patch, n_diag_fields) );
        }
        
        for (unsigned int n_diag_probe = 0; n_diag_probe < PyTools::nComponents("DiagProbe"); n_diag_probe++) {
            vecDiagnostics.push_back( new DiagnosticProbes(params, smpi, n_diag_probe) );
        }
        
        for (unsigned int n_species = 0; n_species < patch->vecSpecies.size(); n_species++) {
            if ( patch->vecSpecies[n_species]->particles->tracked ) {
                vecDiagnostics.push_back( new DiagnosticTrack(params, smpi, patch, n_species) );
            }
        }
        
        return vecDiagnostics;
    } // END createLocalDiagnostics
    
    
    
    static std::vector<ProbeParticles*> createProbes() {
        std::vector<ProbeParticles*> probes(0);
        
        for (unsigned int n_probe = 0; n_probe < PyTools::nComponents("DiagProbe"); n_probe++) {
            probes.push_back( new ProbeParticles() );
        }
        
        return probes;
    } // END createProbes
    
    
    static std::vector<ProbeParticles*> cloneProbes(std::vector<ProbeParticles*> probes) {
        std::vector<ProbeParticles*> newProbes( 0 );
        
        for (int n_probe=0; n_probe<probes.size(); n_probe++) {
            newProbes.push_back( new ProbeParticles(probes[n_probe]) );
        }
        
        return newProbes;
    }
    

};

#endif

