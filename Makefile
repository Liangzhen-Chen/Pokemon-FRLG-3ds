.PHONY: setup download-tools check test test-upstream-local test-native-gpu-local test-native-startup-local build-3ds clean-3ds

setup:
	./scripts/bootstrap.sh

download-tools:
	./scripts/download-test-tools.sh

check:
	./scripts/check-environment.sh

test:
	./tests/smoke/repository.sh
	./scripts/check-upstream-boundary.sh
	./tests/unit/run.sh

test-upstream-local:
	./tests/upstream/run-local.sh

test-native-gpu-local:
	sh ./tests/upstream/run-native-gpu-local.sh

test-native-startup-local:
	sh ./tests/upstream/compile-startup-local.sh

build-3ds:
	$(MAKE) -C platform/3ds

clean-3ds:
	$(MAKE) -C platform/3ds clean
