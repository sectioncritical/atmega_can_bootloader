
ORGNAME=sectioncritical
TAG=avr-build
FULLTAG=$(TAG):latest
REMOTE_TAG=ghcr.io/$(ORGNAME)/$(FULLTAG)

all: build

.PHONY: help
help:
	@echo ""
	@echo "AVR CAN Bootloader Docker Image Maintenance"
	@echo "-------------------------------------------"
	@echo "login       - login to github registry (asks for github access token)"
	@echo "build       - build the avr docker image"
	@echo "push        - push the avr docker image to github"
	@echo "run         - run container for local testing"
	@echo "images      - list local images"
	@echo "containers  - list local containers"
	@echo ""

.PHONY: build
build:
	docker build --progress=plain -t $(TAG) .
	docker tag $(FULLTAG) $(REMOTE_TAG)

.PHONY: login
login:
	docker login -u kroesche ghcr.io

.PHONY: push
push:
	docker push $(REMOTE_TAG)

.PHONY: containers
containers:
	docker container list --all

.PHONY: images
images:
	docker image list --all

.PHONY: run
run:
	cd ../..; docker run -it -v $$(pwd):/project $(TAG) /bin/bash
