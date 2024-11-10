FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y build-essential && \
    apt-get clean

WORKDIR /app

COPY producer.c .
COPY consumer.c .
COPY pathname.txt .

RUN gcc producer.c -o producer -lrt && \
    gcc consumer.c -o consumer -lrt

CMD ["bash", "-c", "./producer & sleep 1 & ./consumer"]
