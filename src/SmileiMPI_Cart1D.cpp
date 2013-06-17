#include "SmileiMPI_Cart1D.h"

#include "Species.h"
#include "ParticleFactory.h"

#include "ElectroMagn.h"
#include "Field1D.h"

#include "Tools.h" 

#include <string>
using namespace std;

SmileiMPI_Cart1D::SmileiMPI_Cart1D( int* argc, char*** argv )
	: SmileiMPI( argc, argv )
{
}

SmileiMPI_Cart1D::SmileiMPI_Cart1D( SmileiMPI* smpi)
	: SmileiMPI( smpi )
{
	ndims_ = 1;
	dims_  = new int(ndims_);
	coords_  = new int(ndims_);
	periods_  = new int(ndims_);
	reorder_ = 0;

	nbNeighbors_ = 2;
	neighbor_  = new int(nbNeighbors_);

	buff_send = new std::vector<Particle*>[nbNeighbors_];
	buff_recv = new std::vector<Particle*>[nbNeighbors_];

	for (int i=0 ; i<ndims_ ; i++) periods_[i] = 0;
	for (int i=0 ; i<ndims_ ; i++) coords_[i] = 0;
	for (int i=0 ; i<ndims_ ; i++) dims_[i] = 0;

	for (int i=0 ; i<nbNeighbors_ ; i++) {
		neighbor_[i] = MPI_PROC_NULL;
		buff_send[i].resize(0);
		buff_recv[i].resize(0);
	}

}

SmileiMPI_Cart1D::~SmileiMPI_Cart1D()
{
	delete dims_;
	delete periods_;
	delete coords_;
	delete neighbor_;

	delete [] buff_send;
	delete [] buff_recv;

	if ( SMILEI_COMM_1D != MPI_COMM_NULL) MPI_Comm_free(&SMILEI_COMM_1D);

}

void SmileiMPI_Cart1D::createTopology()
{
	MPI_Dims_create( smilei_sz, ndims_, dims_ );
	MPI_Cart_create( SMILEI_COMM_WORLD, ndims_, dims_, periods_, reorder_, &SMILEI_COMM_1D );
	MPI_Cart_coords( SMILEI_COMM_1D, smilei_rk, ndims_, coords_ );

	// neighbor_[0]  |  Current process  |  neighbor_[1] //
	MPI_Cart_shift( SMILEI_COMM_1D, 0, 1, &(neighbor_[0]), &(neighbor_[1]) );
	PMESSAGE ( 0, smilei_rk, "Neighbors of process : " << neighbor_[0] << " - " << neighbor_[1]  );

}

void SmileiMPI_Cart1D::exchangeParticles(Species* species, int ispec, PicParams* params)
{
	std::vector<Particle*>* cuParticles = &species->particles;

	//int n_particles = species->getNbrOfParticles();

	//DEBUG( 2, "\tProcess " << smilei_rk << " : " << species->getNbrOfParticles() << " Particles of species " << ispec );
	//MESSAGE( "xmin_local = " << min_local[0] << " - x_max_local = " << max_local[0] );
  
	/********************************************************************************/
	// Build list of particle to exchange
	// Arrays buff_send/buff_recv indexed as array neighbors_
	/********************************************************************************/
	int iPart = 0;
	int n_part_send = indexes_of_particles_to_exchange.size();
	int n_part_recv;
	for (int i=n_part_send-1 ; i>=0 ; i--) {
		iPart = indexes_of_particles_to_exchange[i];
		if      ( (*cuParticles)[iPart]->position(0) < min_local[0]) {
			//DEBUG( smilei_rk << " : Particle to send to west " << iPart << " - x = " << (*cuParticles)[iPart]->position(0) );
			buff_send[0].push_back( (*cuParticles)[iPart] );
			cuParticles->erase(cuParticles->begin()+iPart);
			//! todo{before : species->getNbrOfParticles() = npart_effective, now = particles.size() and getParticlesCapacity() = particles.capacity()}
			// Ne devrait pas être nécessaire : species->getNbrOfParticles()--;      
		}
		else if ( (*cuParticles)[iPart]->position(0) >= max_local[0]) {
			//DEBUG( smilei_rk << " : Particle to send to east " << iPart << " - x = " << (*cuParticles)[iPart]->position(0) );
			buff_send[1].push_back( (*cuParticles)[iPart] );
			cuParticles->erase(cuParticles->begin()+iPart);
			//! todo{before : species->getNbrOfParticles() = npart_effective, now = particles.size() and getParticlesCapacity() = particles.capacity()}
			// Ne devrait pas être nécessaire : species->getNbrOfParticles()--;
		}
	} // END for iPart


	/********************************************************************************/
	// Exchange particles
	/********************************************************************************/
	MPI_Status stat;
	//! \todo{for cartesian grid nbNeighbors_ must be an array 1D/2D/3D of 2 elements}
	//! \todo{add a loop over directions}
	// Loop over neighbors in a direction
	// Send to neighbor_[iNeighbor] / Recv from neighbor_[(iNeighbor+1)%2] :
	// MPI_COMM_SIZE = 2 :  neighbor_[0]  |  Current process  |  neighbor_[1]
	// Rank = 0 : iNeighbor = 0 : neighbor_[0] = NONE : neighbor_[(0+1)%2 = 1
	//            iNeighbor = 1 : neighbor_[1] = 1    : neighbor_[(1+1)%2 = NONE
	// Rank = 1 : iNeighbor = 0 : neighbor_[0] = 0    : neighbor_[(0+1)%2 = NONE
	//            iNeighbor = 1 : neighbor_[1] = NONE : neighbor_[(1+1)%2 = 0
	for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {

		// n_part_send : number of particles to send to current neighbor
		n_part_send = buff_send[iNeighbor].size();
		//DEBUG ( "Neighbor number : " << iNeighbor << " - part to send = " << n_part_send << " to " << neighbor_[iNeighbor] );
		if (neighbor_[iNeighbor]!=MPI_PROC_NULL) {
			MPI_Send( &n_part_send, 1, MPI_INT, neighbor_[iNeighbor], 0, SMILEI_COMM_1D );

			if (n_part_send!=0) {
				//MESSAGE( smilei_rk << " send " << n_part_send << " to process " << neighbor_[iNeighbor] );
				for (int iPart=0 ; iPart<n_part_send; iPart++ ) {
					MPI_Send( &(buff_send[iNeighbor][iPart]->position(0)), 5, MPI_DOUBLE, neighbor_[iNeighbor], 0, SMILEI_COMM_1D );
				}
			}
		} // END of Send

		if (neighbor_[(iNeighbor+1)%2]!=MPI_PROC_NULL) {
			MPI_Recv( &n_part_recv, 1, MPI_INT, neighbor_[(iNeighbor+1)%2], 0, SMILEI_COMM_1D, &stat );
      
			if (n_part_recv!=0) {
				//MESSAGE( smilei_rk << " recv " << n_part_recv << " from process " << neighbor_[(iNeighbor+1)%2] );
				//DEBUG( "Nbr part before recv : " << cuParticles.size() );
				buff_recv[(iNeighbor+1)%2] = ParticleFactory::createVector(params, ispec, n_part_recv);

				//n_particles = species->getNbrOfParticles();
				//cuParticles->resize( n_particles + n_part_recv );

				for (int iPart=0 ; iPart<n_part_recv; iPart++ ) {
					MPI_Recv( &(buff_recv[(iNeighbor+1)%2][iPart]->position(0)), 5, MPI_DOUBLE, neighbor_[(iNeighbor+1)%2], 0, SMILEI_COMM_1D, &stat );
					cuParticles->push_back(buff_recv[(iNeighbor+1)%2][iPart]);
					//! todo{before : species->getNbrOfParticles() = npart_effective, now = particles.size() and getParticlesCapacity() = particles.capacity()}
					// Ne devrait pas être nécessaire : species->getNbrOfParticles()++;
					//cuParticles[n_particles+iPart] = new Particle(ndims_);
					//MPI_Recv( &( cuParticles[n_particles+iPart]->position(0) ), 5, MPI_DOUBLE, neighbor_[(iNeighbor+1)%2], 0, SMILEI_COMM_1D, &stat );
	  
				}
				//MESSAGE( smilei_rk << " recv " <<  n_part_recv << " particules from " << neighbor_[(iNeighbor+1)%2] );
			}
		} // END of Recv

	} // END for iNeighbor

	/********************************************************************************/
	// delete Particles included in buff_send/buff_recv 
	/********************************************************************************/
	for (int i=0 ; i<nbNeighbors_ ; i++) {
		// Particles must be deleted on process sender
		n_part_send =  buff_send[i].size();
		/*for (unsigned int iPart=0 ; iPart<n_part_send; iPart++ ) {
			delete buff_send[i][iPart];
		}*/
		buff_send[i].clear();

		// Not on process receiver, Particles are stored in species
		// Just clean the buffer
		buff_recv[i].clear();
	}

	//DEBUG( 2, "\tProcess " << smilei_rk << " : " << species->getNbrOfParticles() << " Particles of species " << ispec );

} // END exchangeParticles

void SmileiMPI_Cart1D::sumField( Field* field )
{
	std::vector<unsigned int> n_elem = field->dims_;
	Field1D* f1D =  static_cast<Field1D*>(field);

	// Use a buffer per direction to exchange data before summing
	Field1D buf[ nbNeighbors_ ];
	// Size buffer is 2 oversize (1 inside & 1 outside of the current subdomain)
	std::vector<unsigned int> oversize2 = oversize;
	oversize2[0] *= 2; oversize2[0] += 1 + f1D->isPrimal_[0];
	for (int i=0;i<nbNeighbors_ ;i++)  buf[i].allocateDims( oversize2 );

	// istart store in the first part starting index of data to send, then the starting index of data to write in
	// Send point of vue : istart =           iNeighbor * ( n_elem[0]- 2*oversize[0] ) + (1-iNeighbor)       * ( 0 );
	// Rank = 0 : iNeighbor = 0 : send - neighbor_[0] = NONE
	//            iNeighbor = 1 : send - neighbor_[1] = 1 / istart = ( n_elem[0]- 2*oversize[0] )
	// Rank = 1 : iNeighbor = 0 : send - neighbor_[0] = 0 / istart = 0
	//            iNeighbor = 1 : send - neighbor_[1] = NONE
	// Recv point of vue : istart = ( (iNeighbor+1)%2 ) * ( n_elem[0]- 2*oversize[0] ) + (1-(iNeighbor+1)%2) * ( 0 );
	// Rank = 0 : iNeighbor = 0 : recv - neighbor_[1] = 1 / istart = ( n_elem[0]- 2*oversize[0] )
	//            iNeighbor = 1 : recv - neighbor_[0] = NONE
	// Rank = 1 : iNeighbor = 0 : recv - neighbor_[1] = NONE
	//            iNeighbor = 1 : recv - neighbor_[0] = 0 / istart = 0
	int istart;

	MPI_Status stat;

	/********************************************************************************/
	// Send/Recv in a buffer data to sum
	/********************************************************************************/
	// Loop over neighbors in a direction
	// Send to neighbor_[iNeighbor] / Recv from neighbor_[(iNeighbor+1)%2] :
	// See in exchangeParticles()
	for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {

		if (neighbor_[iNeighbor]!=MPI_PROC_NULL) {
			istart = iNeighbor * ( n_elem[0]- oversize2[0] ) + (1-iNeighbor) * ( 0 );
			MPI_Send( &(f1D->data_[istart]), oversize2[0], MPI_DOUBLE, neighbor_[iNeighbor], 0, SMILEI_COMM_1D );
			//cout << "SUM : " << smilei_rk << " send " << oversize2[0] << " data to " << neighbor_[iNeighbor] << " starting at " << istart << endl;
		} // END of Send

		if (neighbor_[(iNeighbor+1)%2]!=MPI_PROC_NULL) {
			istart = ( (iNeighbor+1)%2 ) * ( n_elem[0]- oversize2[0] ) + (1-(iNeighbor+1)%2) * ( 0 );
			MPI_Recv( &( (buf[(iNeighbor+1)%2]).data_[0] ), oversize2[0], MPI_DOUBLE, neighbor_[(iNeighbor+1)%2], 0, SMILEI_COMM_1D, &stat );
			//cout << "SUM : " << smilei_rk << " recv " << oversize2[0] << " data to " << neighbor_[(iNeighbor+1)%2] << " starting at " << istart << endl;
		} // END of Recv

	} // END for iNeighbor

	// Synchro before summing, to not sum with data ever sum
	barrier();
	/********************************************************************************/
	// Sum data on each process, same operation on both side
	/********************************************************************************/
	for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {
		istart = ( (iNeighbor+1)%2 ) * ( n_elem[0]- oversize2[0] ) + (1-(iNeighbor+1)%2) * ( 0 );
		// Using Receiver point of vue
		if (neighbor_[(iNeighbor+1)%2]!=MPI_PROC_NULL) {
		  //cout << "SUM : " << smilei_rk << " sum " << oversize2[0] << " data from " << istart << endl;
			for (unsigned int i=0 ; i<oversize2[0] ; i++)
				f1D->data_[istart+i] += (buf[(iNeighbor+1)%2])(i);
		}
	} // END for iNeighbor


} // END sumField


void SmileiMPI_Cart1D::exchangeField( Field* field )
{
	std::vector<unsigned int> n_elem   = field->dims_;
	std::vector<unsigned int> isPrimal = field->isPrimal_;
	Field1D* f1D =  static_cast<Field1D*>(field);

	// Loop over dimField
	// See sumField for details
	int istart;
	MPI_Status stat;
	// Loop over neighbors in a direction
	// Send to neighbor_[iNeighbor] / Recv from neighbor_[(iNeighbor+1)%2] :
	// See in exchangeParticles()
	for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {

		if (neighbor_[iNeighbor]!=MPI_PROC_NULL) {
			istart = iNeighbor * ( n_elem[0]- (2*oversize[0]+1+isPrimal[0]) ) + (1-iNeighbor) * ( 2*oversize[0]+1-(1-isPrimal[0]) );
			MPI_Send( &(f1D->data_[istart]), 1, MPI_DOUBLE, neighbor_[iNeighbor], 0, SMILEI_COMM_1D );
			//cout << "EXCH : " << smilei_rk << " send " << oversize[0] << " data to " << neighbor_[iNeighbor] << " starting at " << istart << endl;
		} // END of Send

		if (neighbor_[(iNeighbor+1)%2]!=MPI_PROC_NULL) {
			istart = ( (iNeighbor+1)%2 ) * ( n_elem[0] - 1 ) + (1-(iNeighbor+1)%2) * ( 0 )  ;
			MPI_Recv( &(f1D->data_[istart]), 1, MPI_DOUBLE, neighbor_[(iNeighbor+1)%2], 0, SMILEI_COMM_1D, &stat );
			//cout << "EXCH : " << smilei_rk << " recv " << oversize[0] << " data to " << neighbor_[(iNeighbor+1)%2] << " starting at " << istart << endl;
		} // END of Recv

	} // END for iNeighbor

} // END exchangeField


void SmileiMPI_Cart1D::writeField( Field* field, string name )
{
	Field1D* f1D =  static_cast<Field1D*>(field);
	std::vector<unsigned int> n_elem = field->dims_;
	int istart = oversize[0];
	if (smilei_rk!=0) istart+=1;  // f1D_current[n_elem[0]-2*oversize[0]+1] = f1D_west[oversize[0]]
	int bufsize = n_elem[0]- 2*oversize[0] - f1D->isPrimal_[0];

	if (smilei_rk!=0) {
		if (f1D->isPrimal_[0] == 0) bufsize-=1;
		else if (smilei_rk!=smilei_sz-1) bufsize-=1;
	}


	std::ofstream ff;

	for ( int i_rk = 0 ; i_rk < smilei_sz ; i_rk++ ) {
		if (i_rk==smilei_rk) {
			if (smilei_rk==0) ff.open(name.c_str(), ios::out);
			else ff.open(name.c_str(), ios::app);
			//cout << i_rk << " write " << bufsize-1 << " elements from " << istart << " to " << istart+bufsize-1 <<  endl;
			for (int i=istart ; i<istart+bufsize ; i++)
				ff << f1D->data_[i] << endl;
			if (smilei_rk==smilei_sz-1)ff << endl;
			//if (smilei_rk==smilei_sz-1)ff << endl;
			ff.close();
		}
		barrier();
	}


} // END writeField
