# PythonSCAD

OpenSCAD is a very cool tool that lets you express 3D models using its own language. Unfortunately the language itself comes with a lot of intentional limitations.

- No mutation of variables (immutability, "single assignment of any variable")
- Limited number of iterations
- No file I/O

These exist for the reason that they don't want the language to be able to do bad things to people's computers, which allows the "script sharing culture" to be safe.

Additionally the choice to use their own language brings with it a whole new mental model that must be learned and mastered. This is a problem for wide adoption.

**This fork lets you use Python inside of OpenSCAD as its native language** No extra external script to create OpenSCAD code. And as its based on openscad we aim to keep all the features which already exist in openscad. Only added features, no skipped ones ...

Before I continue I'd like to say I fully appreciate all the efforts the team and the Open Source community has contributed towards it over the years. The project is truly a work of love and has brought for many the joy of programming back into their lives. I believe the choice to have a safe script language is a good one.

These limitations cause OpenSCAD programs to be written in the most convoluted ways, making them difficult to understand. While my goal to be able to use Python with OpenSCAD is actually completed, the problem that remains is getting it merged into mainline OpenSCAD.

The argument is Python will introduce a massive security hole into the sharing culture. So the proposed solution is to put the Python capability behind an option, which I have done. Additionally PythonSCAD asks you, if you trust to a new Python Script and it will saves this decsion for you in an SHA256 hash. Now I hope it's just a matter of time until things are merged.

**Update Feb 2025:**
Finally openscad core devs started to merge huge code chunks of PythonSCAD into openscad. So effectively core functionality of PytonSCAD is available in OpenSCAD. This is very exciting as it means the python functionality in OpenSCAD will be available to a way bigger audience.
Finally PythonSCAD will be still available to provide those features, which OpenSCAD will never merge.

A nice tutorial walking you through some exercises can be found [here](tutorial/getting_started.md)
William F. Adams has created a nice wiki on that [here](http://old.reddit.com/r/openpythonscad/wiki/index)
Python Stub files for all available functions in PythonSCAD can be found [here](https://raw.githubusercontent.com/pythonscad/pythonscad/refs/heads/master/libraries/python/openscad.pyi)
