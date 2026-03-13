#!/usr/bin/env python3

import pysu2
from mpi4py import MPI
import numpy as np

def main():
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    
    config_file = "turb_SA_flatplate.cfg"
    
    try:
        if rank == 0:
            print("Initializing SU2 v8.1.0 simulation...")
        
        driver = pysu2.CSinglezoneDriver(config_file, 1, comm)
        
        marker_name = "plate"
        marker_tags = driver.GetMarkerIndices()
        
        if marker_name not in marker_tags:
            if rank == 0:
                print(f"Error: Marker '{marker_name}' not found!")
                print("Available markers:", list(marker_tags.keys()))
            comm.Abort(1)
            return
            
        marker_id = marker_tags[marker_name]
        n_vertices = driver.GetNumberMarkerNodes(marker_id)
        
        if rank == 0:
            print(f"Applying temperature to {n_vertices} vertices on plate")
        
        plate_length = 0.01687
        for i_vert in range(n_vertices):
            coords = driver.MarkerCoordinates(marker_id)
            x_coord = coords(i_vert, 0)
            
            normalized_x = np.clip(x_coord/plate_length, 0.0, 1.0)
            temperature = 300.0 + 200.0 * normalized_x
            
            driver.SetMarkerCustomTemperature(marker_id, i_vert, temperature)
        
        comm.Barrier()
        if rank == 0:
            print("Starting solver...")
        
        driver.StartSolver()
        
        if rank == 0:
            print("Simulation completed successfully")
            
    except Exception as e:
        if rank == 0:
            print(f"Error encountered: {str(e)}")
        comm.Abort(1)
        
    finally:
        if 'driver' in locals():
            del driver

if __name__ == "__main__":
    main()