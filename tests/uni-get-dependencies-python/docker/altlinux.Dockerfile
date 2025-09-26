FROM alt:sisyphus

# Install basic tools
RUN apt-get update && apt-get install -y \
    ca-certificates \
    curl \
    wget \
    python3 \
    python3-module-yaml \
    && apt-get clean

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]