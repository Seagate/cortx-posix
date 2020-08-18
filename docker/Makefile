CENTOS_RELEASE  := 7
NAMESPACE       := registry.gitlab.mero.colo.seagate.com
DOCKER          := docker

INAME = $(@:%-image=%)
CNAME = $(@:%-container=%)

.PHONY: images
images: docker-image-latest \
        docker-image-7.6 \
        docker-image-cortx

.PHONY: docker-image-latest docker-image-7
docker-image-latest docker-image-7: efs-devel-image

.PHONY: docker-image-7.6
docker-image-7.6: CENTOS_RELEASE := 7.6
docker-image-7.6: efs-devel-image

.PHONY: docker-image-cortx
docker-image-cortx: CENTOS_RELEASE := cortx
docker-image-cortx: efs-devel-image

.PHONY: efs-devel-image
efs-devel-image: nfs-ganesha-devel-image
	cd $(INAME) \
	&& $(DOCKER) build \
			--build-arg CENTOS_RELEASE=$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/$(INAME):$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/cortx/fs/efs:$(CENTOS_RELEASE) \
			$(DOCKER_OPTS) .

.PHONY: nfs-ganesha-devel-image
nfs-ganesha-devel-image:
	cd $(INAME) \
	&& $(DOCKER) build \
			--build-arg CENTOS_RELEASE=$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/$(INAME):$(CENTOS_RELEASE) \
			-t $(NAMESPACE)/cortx/fs/nfs-ganesha:$(CENTOS_RELEASE) \
			$(DOCKER_OPTS) .

.PHONY: push
name1 := efs*
name2 := nfs-ganesha*
tag   := *
push:
	@for img in $$(docker images --filter=reference='$(NAMESPACE)/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/$(name2):$(tag)' \
				     --format '{{.Repository}}:{{.Tag}}' | grep -v none) \
		    $$(docker images --filter=reference='$(NAMESPACE)/cortx/fs/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/cortx/fs/$(name2):$(tag)' \
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
		    $$(docker images --filter=reference='$(NAMESPACE)/cortx/fs/$(name1):$(tag)' \
				     --filter=reference='$(NAMESPACE)/cortx/fs/$(name2):$(tag)' \
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
