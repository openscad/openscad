FROM silkeh/solus:base

# Install basic tools
RUN eopkg update-repo && eopkg -y install \
    curl \
    python3 \
    && eopkg delete-cache

# Install PyYAML using ensurepip and pip
RUN python3 -m ensurepip --upgrade && python3 -m pip install PyYAML

# Create working directory
WORKDIR /workspace

# Copy dependency management scripts
COPY scripts/ /workspace/scripts/

# Default command runs package validation for OpenSCAD Qt5
CMD ["python3", "/workspace/scripts/uni-get-dependencies.py", "--dry-run", "--profile", "openscad-qt5"]