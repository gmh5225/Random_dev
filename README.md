### What is it

Linux Kernel module for random numbers

### Getting Started

## Build

1. Firstly, clone this repository:

```bash
git clone git@github.com:ploskiy-2/Random_dev.git
```

2. Then, navigate to the project directory:

```bash
cd Random_dev
```

3. Build object files using Make:

```bash
make
```

4. Insert the module into the kernel, you can choose any parameters:

```bash
sudo insmod Random_Numbers_Module.ko k_length=3 crs_coefficients=2,4,8 crs_elements=10,100,105 crs_c=10
```

5. Output the stream of random bytes:

```bash
sudo xxd /dev/Random_Numbers_Module
```

6. If you want to unload the module:

```bash
sudo rmmod Random_Numbers_Module
make clean
```
