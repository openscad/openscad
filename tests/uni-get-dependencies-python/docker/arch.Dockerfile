FROM archlinux:latest

# Update package database and install basic tools
RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    curl \
    wget \
    ca-certificates \
    python \
    python-yaml \
    && pacman -Scc --noconfirm

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]