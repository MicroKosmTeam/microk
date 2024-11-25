# MicroKosm Module Template
Use this repository as a base for your new MicroK module.  
This contains a build system already set up for use and a basic structure for a module.  
To make it usable:
 - Put it in a subfolder the module/ directory in the main repository  
 - Change the module name in the Makefile  
 - Add compatible Vendor and Product IDs in main.cpp  
 - And start writing code!  

Then, compile from the root directory of the main repository with:  
``make -C module/(moduledirectory) module``
