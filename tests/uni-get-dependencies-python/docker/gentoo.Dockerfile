FROM gentoo/stage3:latest

# Install basic tools
RUN emerge --sync && emerge -q \
    app-misc/ca-certificates \
    net-misc/curl \
    net-misc/wget \
    dev-lang/python:3.12 \
    dev-python/pyyaml \
    && emerge --depclean

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]