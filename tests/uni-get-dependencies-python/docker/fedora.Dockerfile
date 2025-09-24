FROM fedora:40

# Install basic tools
RUN dnf update -y && dnf install -y \
    curl \
    wget \
    ca-certificates \
    python3 \
    python3-pyyaml \
    && dnf clean all

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]