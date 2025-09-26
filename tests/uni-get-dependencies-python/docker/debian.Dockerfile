FROM debian:bookworm

# Install basic tools
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    ca-certificates \
    gnupg \
    python3 \
    python3-yaml \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Set environment for testing
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]