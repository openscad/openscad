#!/bin/bash

set -e

# Test runner for Docker-based dependency validation
# This script builds and runs Docker containers for all supported distributions
# and reports on package availability issues

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOCKER_DIR="$SCRIPT_DIR/docker"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Distribution list
DISTRIBUTIONS=(
    "debian"
    "ubuntu"
    "fedora"
    "arch"
    "opensuse"
    "gentoo"
    "mageia"
    "solus"
    "altlinux"
)

# Results tracking
declare -A results
declare -A build_status
declare -A run_status

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Build Docker image for a distribution
build_image() {
    local distro="$1"
    local verbose="$2"
    local dockerfile="$DOCKER_DIR/$distro.Dockerfile"
    local image_name="openscad-deps-test:$distro"

    if [[ ! -f "$dockerfile" ]]; then
        log_error "Dockerfile not found: $dockerfile"
        build_status["$distro"]="missing_dockerfile"
        return 1
    fi

    log_info "Building Docker image for $distro..."

    if [[ "$verbose" == "true" ]]; then
        echo -e "${BLUE}[VERBOSE]${NC} Running: docker build -t $image_name -f $dockerfile $PROJECT_ROOT"
        if docker build -t "$image_name" -f "$dockerfile" "$PROJECT_ROOT"; then
            log_success "Built image for $distro"
            build_status["$distro"]="success"
            return 0
        else
            log_error "Failed to build image for $distro"
            build_status["$distro"]="build_failed"
            return 1
        fi
    else
        if docker build -t "$image_name" -f "$dockerfile" "$PROJECT_ROOT" &>/dev/null; then
            log_success "Built image for $distro"
            build_status["$distro"]="success"
            return 0
        else
            log_error "Failed to build image for $distro"
            build_status["$distro"]="build_failed"
            return 1
        fi
    fi
}

# Run container and capture results
run_test() {
    local distro="$1"
    local verbose="$2"
    local image_name="openscad-deps-test:$distro"

    if [[ "${build_status["$distro"]}" != "success" ]]; then
        log_warning "Skipping $distro test - build failed"
        run_status["$distro"]="skipped_build_failed"
        return 1
    fi

    log_info "Running dependency test for $distro..."

    # Run container and capture output
    local output
    local exit_code

    if [[ "$verbose" == "true" ]]; then
        echo -e "${BLUE}[VERBOSE]${NC} Running: docker run --rm $image_name"
        echo -e "${BLUE}[VERBOSE]${NC} === Output for $distro ==="
        if docker run --rm "$image_name"; then
            exit_code=0
            log_success "Dependency test passed for $distro"
            run_status["$distro"]="success"
            results["$distro"]="(output shown above)"
        else
            exit_code=$?
            log_error "Dependency test failed for $distro (exit code: $exit_code)"
            run_status["$distro"]="test_failed"
            results["$distro"]="(output shown above)"
        fi
        echo -e "${BLUE}[VERBOSE]${NC} === End output for $distro ==="
        echo
    else
        if output=$(docker run --rm "$image_name" 2>&1); then
            exit_code=0
            log_success "Dependency test passed for $distro"
            run_status["$distro"]="success"
            results["$distro"]="$output"
        else
            exit_code=$?
            log_error "Dependency test failed for $distro (exit code: $exit_code)"
            run_status["$distro"]="test_failed"
            results["$distro"]="$output"
        fi
    fi

    return $exit_code
}

# Clean up Docker images
cleanup() {
    log_info "Cleaning up Docker images..."
    for distro in "${DISTRIBUTIONS[@]}"; do
        if [[ "${build_status["$distro"]}" == "success" ]]; then
            docker rmi "openscad-deps-test:$distro" &>/dev/null || true
        fi
    done
}

# Generate summary report
generate_report() {
    echo
    echo "=========================================="
    echo "DEPENDENCY VALIDATION TEST SUMMARY"
    echo "=========================================="
    echo

    # Count results
    local total=${#DISTRIBUTIONS[@]}
    local build_success=0
    local test_success=0
    local build_failed=0
    local test_failed=0

    for distro in "${DISTRIBUTIONS[@]}"; do
        case "${build_status["$distro"]}" in
            "success") ((build_success++)) ;;
            "build_failed"|"missing_dockerfile") ((build_failed++)) ;;
        esac

        case "${run_status["$distro"]}" in
            "success") ((test_success++)) ;;
            "test_failed") ((test_failed++)) ;;
        esac
    done

    echo "Build Results:"
    echo "  ✓ Successful builds: $build_success/$total"
    echo "  ✗ Failed builds: $build_failed/$total"
    echo
    echo "Test Results:"
    echo "  ✓ Successful tests: $test_success/$total"
    echo "  ✗ Failed tests: $test_failed/$total"
    echo

    # Detailed results
    echo "Detailed Results:"
    echo "-----------------"
    for distro in "${DISTRIBUTIONS[@]}"; do
        local build_result="${build_status["$distro"]:-unknown}"
        local test_result="${run_status["$distro"]:-unknown}"

        printf "%-12s Build: %-20s Test: %s\n" "$distro" "$build_result" "$test_result"
    done
    echo

    # Show failures in detail
    echo "Failed Tests Output:"
    echo "-------------------"
    for distro in "${DISTRIBUTIONS[@]}"; do
        if [[ "${run_status["$distro"]}" == "test_failed" ]]; then
            echo
            echo "=== $distro ==="
            echo "${results["$distro"]}"
        fi
    done

    # Exit with error if any tests failed
    if (( test_failed > 0 || build_failed > 0 )); then
        return 1
    else
        return 0
    fi
}

# Main execution
main() {
    local cleanup_on_exit=true
    local parallel=false
    local verbose=false

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --no-cleanup)
                cleanup_on_exit=false
                shift
                ;;
            --parallel)
                parallel=true
                shift
                ;;
            --verbose|-v)
                verbose=true
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --no-cleanup    Don't remove Docker images after testing"
                echo "  --parallel      Run tests in parallel (experimental)"
                echo "  --verbose, -v   Show real-time Docker build and run output"
                echo "  --help, -h      Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        exit 1
    fi

    # Check if Docker daemon is running
    if ! docker info &>/dev/null; then
        log_error "Docker daemon is not running"
        exit 1
    fi

    log_info "Starting dependency validation tests for ${#DISTRIBUTIONS[@]} distributions"
    echo

    # Build all images first
    log_info "Building Docker images..."
    for distro in "${DISTRIBUTIONS[@]}"; do
        build_image "$distro" "$verbose"
    done

    echo
    log_info "Running dependency tests..."

    # Run tests
    for distro in "${DISTRIBUTIONS[@]}"; do
        run_test "$distro" "$verbose"
    done

    # Generate report
    generate_report
    local report_exit_code=$?

    # Cleanup if requested
    if [[ "$cleanup_on_exit" == "true" ]]; then
        cleanup
    fi

    exit $report_exit_code
}

# Set up signal handlers for cleanup
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    trap cleanup EXIT INT TERM
    main "$@"
fi