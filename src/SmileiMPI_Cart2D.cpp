#include "SmileiMPI_Cart2D.h"

#include "Species.h"
#include "ParticleFactory.h"

#include "ElectroMagn.h"
#include "Field2D.h"

#include "Tools.h" 

#include <string>
#include <mpi.h>
using namespace std;

SmileiMPI_Cart2D::SmileiMPI_Cart2D( int* argc, char*** argv )
	: SmileiMPI( argc, argv )
{
}

SmileiMPI_Cart2D::SmileiMPI_Cart2D( SmileiMPI* smpi)
	: SmileiMPI( smpi )
{
	ndims_ = 2;
	dims_  = new int(ndims_);
	coords_  = new int(ndims_);
	periods_  = new int(ndims_);
	reorder_ = 0;

	nbNeighbors_ = 2; // per direction
	//neighbor_  = new int(nbNeighbors_);

	buff_send = new std::vector<Particle*>[nbNeighbors_];
	buff_recv = new std::vector<Particle*>[nbNeighbors_];

	for (int i=0 ; i<ndims_ ; i++) periods_[i] = 0;
	for (int i=0 ; i<ndims_ ; i++) coords_[i] = 0;
	for (int i=0 ; i<ndims_ ; i++) dims_[i] = 0;

	for (int iDim=0 ; iDim<ndims_ ; iDim++)
		for (int iNeighbors=0 ; iNeighbors<nbNeighbors_ ; iNeighbors++)
			neighbor_[iDim][iNeighbors] = MPI_PROC_NULL;

	for (int i=0 ; i<nbNeighbors_ ; i++) {
		buff_send[i].resize(0);
		buff_recv[i].resize(0);
	}
}

SmileiMPI_Cart2D::~SmileiMPI_Cart2D()
{
	for (int ix_isPrim=0 ; ix_isPrim<1 ; ix_isPrim++) {
		for (int iy_isPrim=0 ; iy_isPrim<1 ; iy_isPrim++) {
			MPI_Type_free( &ntype_[0][ix_isPrim][iy_isPrim]); //line
			MPI_Type_free( &ntype_[1][ix_isPrim][iy_isPrim]); // column

			MPI_Type_free( &ntypeSum_[0][ix_isPrim][iy_isPrim]); //line
			MPI_Type_free( &ntypeSum_[1][ix_isPrim][iy_isPrim]); // column
		}
	}

	delete dims_;
	delete periods_;
	delete coords_;
	//delete neighbor_;

	delete [] buff_send;
	delete [] buff_recv;

	if ( SMILEI_COMM_2D != MPI_COMM_NULL) MPI_Comm_free(&SMILEI_COMM_2D);

}

void SmileiMPI_Cart2D::createTopology()
{
	dims_[0] = number_of_procs[0];
	dims_[1] = number_of_procs[1];

	MPI_Cart_create( SMILEI_COMM_WORLD, ndims_, dims_, periods_, reorder_, &SMILEI_COMM_2D );
	MPI_Cart_coords( SMILEI_COMM_2D, smilei_rk, ndims_, coords_ );

	// neighbor_[0]  |  Current process  |  neighbor_[1] //
	for (int iDim=0 ; iDim<ndims_ ; iDim++) {
		MPI_Cart_shift( SMILEI_COMM_2D, iDim, 1, &(neighbor_[iDim][0]), &(neighbor_[iDim][1]) );
		PMESSAGE ( 0, smilei_rk, "Neighbors of process in direction " << iDim << " : " << neighbor_[iDim][0] << " - " << neighbor_[iDim][1]  );
	}

}

void SmileiMPI_Cart2D::exchangeParticles(Species* species, int ispec, PicParams* params)
{
	MESSAGE( "to be implemented" );

} // END exchangeParticles


void SmileiMPI_Cart2D::createType( PicParams& params )
{
	int nx0 = params.n_space[0] + 1 + 2*params.oversize[0];
	int ny0 = params.n_space[1] + 1 + 2*params.oversize[1];

	// MPI_Datatype ntype_[nDim][primDual][primDual]
	int nx, ny;
	int nline, ncol;
	for (int ix_isPrim=0 ; ix_isPrim<2 ; ix_isPrim++) {
		nx = nx0 + ix_isPrim;
		for (int iy_isPrim=0 ; iy_isPrim<2 ; iy_isPrim++) {
			ny = ny0 + iy_isPrim;
			ntype_[0][ix_isPrim][iy_isPrim] = NULL;
			MPI_Type_contiguous(ny, MPI_DOUBLE, &(ntype_[0][ix_isPrim][iy_isPrim]));    //line
			MPI_Type_commit( &(ntype_[0][ix_isPrim][iy_isPrim]) );
			ntype_[1][ix_isPrim][iy_isPrim] = NULL;
			MPI_Type_vector(nx, 1, ny, MPI_DOUBLE, &(ntype_[1][ix_isPrim][iy_isPrim])); // column
			MPI_Type_commit( &(ntype_[1][ix_isPrim][iy_isPrim]) );

			ntypeSum_[0][ix_isPrim][iy_isPrim] = NULL;
			nline = 1 + 2*params.oversize[0] + ix_isPrim;
			MPI_Type_contiguous(nline, ntype_[0][ix_isPrim][iy_isPrim], &(ntypeSum_[0][ix_isPrim][iy_isPrim]));    //line
			MPI_Type_commit( &(ntypeSum_[0][ix_isPrim][iy_isPrim]) );
			ntypeSum_[1][ix_isPrim][iy_isPrim] = NULL;
			ncol  = 1 + 2*params.oversize[1] + iy_isPrim;
			MPI_Type_vector(nx, ncol, ny, MPI_DOUBLE, &(ntypeSum_[1][ix_isPrim][iy_isPrim])); // column
			MPI_Type_commit( &(ntypeSum_[1][ix_isPrim][iy_isPrim]) );

		}
	}

}


void SmileiMPI_Cart2D::sumField( Field* field )
{
	std::vector<unsigned int> n_elem = field->dims_;
	std::vector<unsigned int> isPrimal = field->isPrimal_;
	Field2D* f2D =  static_cast<Field2D*>(field);


	// Use a buffer per direction to exchange data before summing
	Field2D buf[ndims_][ nbNeighbors_ ];
	// Size buffer is 2 oversize (1 inside & 1 outside of the current subdomain)
	std::vector<unsigned int> oversize2 = oversize;
	oversize2[0] *= 2; oversize2[0] += 1 + f2D->isPrimal_[0];
	oversize2[1] *= 2; oversize2[1] += 1 + f2D->isPrimal_[1];

	for (int iDim=0 ; iDim<ndims_ ; iDim++) {
		for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {
			std::vector<unsigned int> tmp(ndims_,0);
			tmp[0] =    iDim  * n_elem[0] + (1-iDim) * oversize2[0];
			tmp[1] = (1-iDim) * n_elem[1] +    iDim  * oversize2[1];
			buf[iDim][iNeighbor].allocateDims( tmp );
		}
	}

	int istart, ix, iy;
	MPI_Status stat;

	/********************************************************************************/
	// Send/Recv in a buffer data to sum
	/********************************************************************************/
	for (int iDim=0 ; iDim<ndims_ ; iDim++) {

		MPI_Datatype ntype = ntypeSum_[iDim][isPrimal[0]][isPrimal[1]];

		for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {

			if (neighbor_[iDim][iNeighbor]!=MPI_PROC_NULL) {
				istart = iNeighbor * ( n_elem[iDim]- oversize2[iDim] ) + (1-iNeighbor) * ( 0 );
				ix = (1-iDim)*istart;
				iy =    iDim *istart;
				MPI_Send( &(f2D->data_[ix][iy]), 1, ntype, neighbor_[iDim][iNeighbor], 0, SMILEI_COMM_2D );
			} // END of Send

			if (neighbor_[iDim][(iNeighbor+1)%2]!=MPI_PROC_NULL) {
				int tmp_elem = (buf[iDim][(iNeighbor+1)%2]).dims_[0]*(buf[iDim][(iNeighbor+1)%2]).dims_[1];
				MPI_Recv( &( (buf[iDim][(iNeighbor+1)%2]).data_[0][0] ), tmp_elem, MPI_DOUBLE, neighbor_[iDim][(iNeighbor+1)%2], 0, SMILEI_COMM_2D, &stat );
			} // END of Recv

		} // END for iNeighbor

	} // END for iDim

	// Synchro before summing, to not sum with data ever sum
	barrier();
	/********************************************************************************/
	// Sum data on each process, same operation on both side
	/********************************************************************************/
	for (int iDim=0 ; iDim<ndims_ ; iDim++) {
		for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {
			istart = ( (iNeighbor+1)%2 ) * ( n_elem[iDim]- oversize2[iDim] ) + (1-(iNeighbor+1)%2) * ( 0 );
			int ix0 = (1-iDim)*istart;
			int iy0 =    iDim *istart;
			if (neighbor_[iDim][(iNeighbor+1)%2]!=MPI_PROC_NULL) {
				for (unsigned int ix=0 ; ix< (buf[iDim][(iNeighbor+1)%2]).dims_[0] ; ix++) {
					for (unsigned int iy=0 ; iy< (buf[iDim][(iNeighbor+1)%2]).dims_[1] ; iy++)
						f2D->data_[ix0+ix][iy0+iy] += (buf[iDim][(iNeighbor+1)%2])(ix,iy);
				}
			} // END if

		} // END for iNeighbor
	} // END for iDim

} // END sumField


void SmileiMPI_Cart2D::exchangeField( Field* field )
{
	std::vector<unsigned int> n_elem   = field->dims_;
	std::vector<unsigned int> isPrimal = field->isPrimal_;
	Field2D* f2D =  static_cast<Field2D*>(field);

	MPI_Status stat;

	int istart, ix, iy;

	// Loop over dimField
	for (int iDim=0 ; iDim<ndims_ ; iDim++) {

		MPI_Datatype ntype = ntype_[iDim][isPrimal[0]][isPrimal[1]];
		for (int iNeighbor=0 ; iNeighbor<nbNeighbors_ ; iNeighbor++) {

			if (neighbor_[iDim][iNeighbor]!=MPI_PROC_NULL) {

				istart = iNeighbor * ( n_elem[iDim]- (2*oversize[iDim]+1+isPrimal[iDim]) ) + (1-iNeighbor) * ( 2*oversize[iDim]+1-(1-isPrimal[iDim]) );
				ix = (1-iDim)*istart;
				iy =    iDim *istart;
				MPI_Send( &(f2D->data_[ix][iy]), 1, ntype, neighbor_[iDim][iNeighbor], 0, SMILEI_COMM_2D );

			} // END of Send

			if (neighbor_[iDim][(iNeighbor+1)%2]!=MPI_PROC_NULL) {

				istart = ( (iNeighbor+1)%2 ) * ( n_elem[iDim] - 1 ) + (1-(iNeighbor+1)%2) * ( 0 )  ;
				ix = (1-iDim)*istart;
				iy =    iDim *istart;
				MPI_Recv( &(f2D->data_[ix][iy]), 1, ntype, neighbor_[iDim][(iNeighbor+1)%2], 0, SMILEI_COMM_2D, &stat );

			} // END of Recv

		} // END for iNeighbor

	} // END for iDim


} // END exchangeField


void SmileiMPI_Cart2D::writeField( Field* field, string name )
{
	MESSAGE( "to be implemented" );

} // END writeField