FROM mageia:9

# Install basic tools
RUN urpmi.update -a && urpmi --auto \
    rootcerts \
    curl \
    wget \
    python3 \
    python3-yaml

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]