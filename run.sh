#!/bin/bash
set -e
echo "Build C++ agent (requires mysql connector libs available)..."
mkdir -p build
pushd build
cmake ../cpp
make
popd
echo "Copy binary to /usr/local/bin (you may need sudo)"
cp build/cloud_monitor_agent /usr/local/bin/
echo "You can run: /usr/local/bin/cloud_monitor_agent &"
echo "Then run Python integrator:"
echo "python3 python/integrator.py --config config/sample_config.json"
