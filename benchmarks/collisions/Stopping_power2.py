# ---------------------------------------------
# SIMULATION PARAMETERS FOR THE PIC-CODE SMILEI
# ---------------------------------------------

import math
L0 = 2.*math.pi # conversion from normalization length to wavelength


Main(
    geometry = "1d3v",

    number_of_patches = [ 4 ],

    interpolation_order = 2,

    timestep = 200. * L0,
    sim_time = 5000 * L0,


    time_fields_frozen = 100000000000.,

    cell_length = [2000.*L0],
    sim_length = [40000.*L0],

    bc_em_type_x = ["periodic"],


    random_seed = 0,

	referenceAngularFrequency_SI = L0 * 3e8 /1.e-6,
    print_every = 10,
)


Species(
	species_type = "backgroundelectron",
	initPosition_type = "regular",
	initMomentum_type = "maxwell-juettner",
	ionization_model = "none",
	n_part_per_cell = 10000,
	mass = 1.,
	charge = -1.0,
	charge_density = 10.,
	mean_velocity = [0., 0., 0.],
	temperature = [0.01],
	time_frozen = 100000000.0,
	bc_part_type_west = "none",
	bc_part_type_east = "none"
)

Species(
	species_type = "electron1",
	initPosition_type = "regular",
	initMomentum_type = "maxwell-juettner",
	n_part_per_cell= 10000,
	mass = 1.0,
	charge = -1.0,
	charge_density = trapezoidal(0.00001, xplateau=20000.*L0),
	mean_velocity = [0.55, 0., 0.],
	temperature = [0.0000001],
	time_frozen = 100000000.0,
	bc_part_type_west = "none",
	bc_part_type_east = "none"
)

Species(
	species_type = "electron2",
	initPosition_type = "regular",
	initMomentum_type = "maxwell-juettner",
	n_part_per_cell= 10000,
	mass = 1.0,
	charge = -1.0,
	charge_density = trapezoidal(0.00001, xvacuum=20000.*L0, xplateau=20000.*L0),
	mean_velocity = [0.776, 0., 0.],
	temperature = [0.0000001],
	time_frozen = 100000000.0,
	bc_part_type_west = "none",
	bc_part_type_east = "none"
)

Collisions(
	species1 = ["backgroundelectron"],
	species2 = ["electron1", "electron2"],
	coulomb_log = 3.
)




DiagFields(
	every = 1
)


DiagScalar(
	every = 1
)



DiagParticles(
	output = "density",
	every = 2,
	time_average = 1,
	species = ["electron1", "electron2"],
	axes = [
		 ["x",    0*L0,    40000.*L0,   2],
		 ["ekin",   0.1,  1,   500, "logscale"]
	]
)
