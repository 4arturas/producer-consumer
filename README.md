### Check for existing semaphore sets using the command:
```sh
ipcs -s
````

### Remove it using:
```sh
ipcrm -s <semid>
````

```sh
docker build -t producer-consumer -f Dockerfile .
````

```sh
docker run --rm -it --privileged producer-consumer
````