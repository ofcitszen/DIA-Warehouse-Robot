# DIA-Warehouse-Robot

To run the simulation, drag or copy the warehouse_resources folder into the x64/Debug folder and then open the .exe file. You may need to grant permission to the .exe file for it to run. You will also need C++ installed on your device to run it.

------------------------------------------------------------------------------------

In the main menu:
- Press "Start" to start a single simulation for the chosen Settings.
- Press "Test All" to run 10 simulations for all combinations of settings and output the results into a text file in the same folder as the .exe file.
- Press "Settings" to change the Settings.
- Press "Quit" to close the program.

In the simulation:
- Press W to zoom out.
- Press E to zoom in.
- Press R to reset the camera position and zoom
- Use the arrow keys to move the camera.
- Press TAB to toggle between the view of the map known by the robots (default) and the view of the actual map.
- Press ESC to pause.
	- Under the pause menu:
		- Press "Resume" to continue the simulation.
		- Press "Menu" to return to the main menu. (Disabled in "Test All" mode)
		- Press "Finish" to prematurely finish this simulation and get the results.
		- Press "Quit" to quit the program entirely.

Exclusive to the "Test All" mode:
- Once in the simulation, press SPACE BAR to skip all simulations for the current setting.
- Simulations will fail if the number of ticks taken exceeds 5000.
- Failed simulations (either prematurely ended, skipped or simulations that are impossible or take too long to finish) will not have their results counted in the average results in the text file.

You can see which setting is currently being used by looking at the print statements in the terminal.

------------------------------------------------------------------------------------

The source code of the simulation is contained within Warehouse Robot Simulation.cpp.

Unfortunately, this simulation is only available for Windows OS devices because the program file is a .exe file. To run it on other operating systems, you may consider using other tools to run it, such as by using Boot Camp to run Windows OS on a Mac.