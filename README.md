# ViperViz
Visualization and logging software for the Polhemus Viper electromagnetic tracking system

## How to install (for Mac)

### 1. Clone the code to your computer

```bash
git clone https://github.com/margindx/ViperViz.git
cd ViperViz
```
### 2. Download dependencies

```bash
brew install cmake
brew install pkg-config
brew install raylib
brew install zmq
brew install cppzmq
brew install libusb
```

### 3. Compile the binaries

```bash
mkdir build
cd build
cmake ..
make
```

### 4. Run the program

```
./viper
```

## How to run in a Docker container

To run the app in a Docker container, the Docker engine must be installed on
your system.  See [Docker documentation](https://docs.docker.com/engine/install)
for available installation methods.

Start the app with

```bash
docker compose up
```

The container exposes port 5555 that subscribers can use to access the data
stream.

Terminate the app with

```bash
docker compose down
```
