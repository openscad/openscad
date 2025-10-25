OpenSCAD Release Policy
-----------------------

The OpenSCAD release process is an automated, time-bound process. We achieve this by writing the
release blockers into the CI process, so that code merged to master is ready to release on time, and
by placing a feature freeze on the last month before each release.

Each cycle has two phases:

 - Development (2 months): New features and changes are merged into the main branch.
 - Feature Freeze (1 month): Focus shifts to bug fixing, documentation, and translation. Features 
   with significant unaddressed bugs may be disabled or reverted at the maintainers' discretion:
   there is no blocking the release.

| Version | Feature Development            | Feature Freeze | Release (automated) |
|---------|--------------------------------|----------------|---------------------|
| YYYY.02 | December (Prev. Year), January | February       | February 28th       |
| YYYY.05 | March, April                   | May            | May 31st            |
| YYYY.08 | June, July                     | August         | August 31st         |
| YYYY.11 | September, October             | November       | November 30th       |

## Release process

The morning of the release day, a Github Action will:

1. Generate a `RELEASE_NOTES.md` file using the `makereleasenotes.sh` script, and commit the changes. 
2. Tag the release and push it to github. This will trigger the CircleCI workflow to build and push the release
   artifacts to the GitHub releases.
3. Push a `release-YYYY.MM` branch

The release CircleCI action will:

1. Build binaries for each target platform
2. Call a webhook on the file server every 5 minutes until it returns success to ensure that all artifacts have been downloaded and stored in the right places
3. Push a commit to openscad.github.com to update the `*_release_links.js` files

The cron job running on the file server will

On the day after the release, the maintainer will, using the `releases/YYYY.MM.md` document as a
reference, announce the new release:

- Update web page
    - news.html
- Update external resources:
    - https://en.wikipedia.org/wiki/OpenSCAD
- Write to mailing list
- Tweet as OpenSCAD
- Notify package managers
    - Debian/Ubuntu: https://launchpad.net/~chrysn
    - Ubuntu PPA: https://github.com/hyperair
    - Fedora: Miro Hrončok <miro@hroncok.cz> or <mhroncok@redhat.com>
    - OpenSUSE: Pavol Rusnak <prusnak@opensuse.org>
    - Arch Linux: Kyle Keen <keenerd@gmail.com>
    - MacPorts: https://svn.macports.org/repository/macports/trunk/dports/science/openscad/Portfile
    - Homebrew: https://github.com/caskroom/homebrew-cask/blob/master/Casks/openscad.rb
- Make & merge PRs to the Flatpak and Snap repos bumping the version number
