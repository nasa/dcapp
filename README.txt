
TODO tidy this up

macos dependencies:
brew
vulkansdk
> brew install gdal libxml2

linux dependencies (rhel):
vulkansdk
> sudo dnf install gdal-devel libxml2-devel

steps:
1) build pilotlight (debug_experimental)
2) gen build script (python3 scripts/gen-build.py)
3) build dcapp (./scripts/build-dcapp.sh)
4) build samples (./scripts/build-samples.py)
5) run dcapp (./start-dcapp.py)

ORDER OF EXECUTION (legacy):
* reads device inputs (bezel buttons, keyboard?)
* process comm interfaces (trick, edge)
*** write, then read
* sets current panel index
* runs display logic (.cpp files)
* runs animators
* potentially runs display logic again???
* gets mouse position, process mouse events
*** pass mouse positions to each node
*** use mouse actions for events (click, keyclick, close, etc.)
* updates pixelstreams?
* draw nodes
* swaps buffers
*** for double buffering
