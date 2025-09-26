FROM opensuse/tumbleweed:latest

# Install basic tools
RUN zypper refresh && zypper install -y \
    curl \
    wget \
    ca-certificates \
    python3 \
    python3-PyYAML \
    && zypper clean -a

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]