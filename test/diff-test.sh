#!/bin/bash

./bic-myfind /var/tmp/test-find/simple -print &> ./1.out
../output/Debug/myfind /var/tmp/test-find/simple -print &> ./2.out
meld ./1.out ./2.out &