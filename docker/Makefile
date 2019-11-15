CENTOS_RELEASE  := 7
NAMESPACE       := registry.gitlab.mero.colo.seagate.com
DOCKER          := docker

INAME = $(@:%-image=%)
CNAME = $(@:%-container=%)

.PHONY: images
images: docker-image-latest \
        docker-image-7.6 \
        docker-image-eos

.PHONY: docker-image-latest
docker-image-latest: eos-fs-devel-image

.PHONY: docker-image-7.6
docker-image-7.6: CENTOS_RELEASE := 7.6
docker-image-7.6: eos-fs-devel-image

.PHONY: docker-image-eos
docker-image-eos: CENTOS_RELEASE := eos
docker-image-eos: eos-fs-devel-image

.PHONY: eos-fs-devel-image
eos-fs-devel-image: nfs-ganesha-devel-image
	cd $(INAME) \
	&& $(DOCKER) build \
			--build-arg CENTOS_RELEASE=$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/$(INAME):$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/eos/fs/eos-fs:$(CENTOS_RELEASE) \
			$(DOCKER_OPTS) .

.PHONY: nfs-ganesha-devel-image
nfs-ganesha-devel-image:
	cd $(INAME) \
	&& $(DOCKER) build \
			--build-arg CENTOS_RELEASE=$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/$(INAME):$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/eos/fs/nfs-ganesha:$(CENTOS_RELEASE) \
			$(DOCKER_OPTS) .

.PHONY: push
name1 := eos-fs*
name2 := nfs-ganesha*
tag   := *
push:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/$(name2):$(tag)' \
				     --format '{{.Repository}}:{{.Tag}}' | grep -v none) \
		    $$(docker images --filter=reference='$(NAMESPACE)/eos/fs/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/eos/fs/$(name2):$(tag)' \
				     --format '{{.Repository}}:{{.Tag}}' | grep -v none) ; \
	do \
		echo "---> $$img" ; \
		$(DOCKER) push $$img ; \
	done

.PHONY: clean
clean:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/$(name2):$(tag)' \
				     --format '{{.Repository}}:{{.Tag}}') \
		    $$(docker images --filter=reference='$(NAMESPACE)/eos/fs/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/eos/fs/$(name2):$(tag)' \
				     --format '{{.Repository}}:{{.Tag}}') ; \
	do \
		echo "---> $$img" ; \
		$(DOCKER) rmi $$img ; \
	done

.PHONY: help
help:
	@echo '  images  - create docker images for CI environment'
	@echo "  push    - upload local $(NAMESPACE)/$(name1)* images to docker registry"
	@echo "  clean   - remove local $(NAMESPACE)/$(name1)* images"
