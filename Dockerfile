# syntax=docker/dockerfile:1
FROM ubuntu:noble AS build-stage
WORKDIR /usr/local/src

# Update the system and install dependencies.
RUN apt update -y && apt upgrade -y && apt install -y \
    build-essential \
    cmake \
    cppzmq-dev \
    git \
    libasound2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libx11-dev \
    libxi-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxkbcommon-dev \
    libxrandr-dev \
    libwayland-dev \
    libusb-1.0-0-dev \
    nlohmann-json3-dev \
    pkg-config 

# Build and install raylib.
RUN git clone https://github.com/raysan5/raylib.git \
    && cd raylib \
    && cmake -D BUILD_SHARED_LIBS=ON -B build -S . \
    && cmake --build build \
    && cmake --install build

# Build and install ViperViz.
COPY . viperviz
RUN cd viperviz \
    && cmake -B build -S . \
    && cmake --build build \
    && cmake --install build

FROM ubuntu:noble AS final-stage
RUN apt update -y && apt upgrade -y && apt install -y libusb-1.0-0 libzmq5
COPY --from=build-stage /usr/local/bin /usr/local/bin
COPY --from=build-stage /usr/local/include /usr/local/include
COPY --from=build-stage /usr/local/lib /usr/local/lib
RUN ldconfig

ENTRYPOINT ["viper"]
