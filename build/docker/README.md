Custom Docker Image for CI
==========================

What is this for?

In order to run the build and test operations for this project, a certain set
of utilities are needed in the docker image. There is no pre-existing image
that has everything needed. To resolve this I can:

- break up CI pipeline into steps with each step using a different image
- add extra `apt-get install` commands in the pipeline script (takes longer)
- prepare my own docker image that has exactly the tools needed

The Dockerfile here can be used to build an image that is used for this
project's CI operations.

The image, once built, needs to be pushed to the github container registry.
These are the steps to follow:

### Login

Using your github credentials:

    (sudo) docker login ghcr.io -u <username>

you will be prompted for the password, which is a Github personal access
token. You can also pass the token on the command line through and environment
variable.

    export GITHUB_PERSONAL_TOKEN=(the token value)
    echo $GITHUB_PERSONAL_TOKEN | docker login ghcr.io -u USERNAME --password-stdin

### Build

    (sudo) docker build -t ghcr.io/ORGNAME/avr-build .

ORGNAME is a Github user name or organization name.

### Push

    (sudo) docker push ghcr.io/ORGNAME/avr-build:latest

### Usage

In the Github workflow file, use this container:

    ghcr.io/ORGNAME/avr-build:latest

Notes
-----

- Depending on your docker setup, you may or may not need `sudo`.

- After the build step, it is a good idea to run it locally and exercise the
  container to make sure it has all the packages you need and executes the
  build correctly.

- If you have 2FA enabled on the account you will need to set up a personal
  access token for authentication.

- It would be a good idea to automate all this as a separate job, but not
  worth it for now.

- For more info, google `github ci container registry`
