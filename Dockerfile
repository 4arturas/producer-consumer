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

#COPY producer_consumer_processes.c .
# RUN gcc -o producer_consumer_processes producer_consumer_processes.c
#RUN gcc -o producer_consumer_processes producer_consumer_processes.c -lpthread


#COPY test.c .
#RUN gcc -o producer_consumer_processes test.c


#CMD ["bash", "-c", "./producer_consumer_processes"]
#CMD ["bash", "-c", "./producer & sleep 1 & ./consumer"]
CMD ["bash", "-c", "./producer & sleep 1 & ./consumer & ./consumer & ./consumer"]


