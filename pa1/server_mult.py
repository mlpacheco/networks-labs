import socket
import sys
import os
import re
import threading

BUFFER_SIZE = 10000000
FIXED_PORT = 6000

# validate http request by extracting the GET request
# line and parsing it
def validate_http_request(request):
    match = re.search(r'(.*?\r\n)', request)
    if not match:
        return False, None

    firstline = match.group(1)
    splits = firstline.split()

    if  len(splits) != 3 or \
        splits[2] != "HTTP/1.1" or \
        splits[0] != "GET" or \
        not splits[1].startswith("/"):

        return False, None

    filename = splits[1][1:]
    if splits[1] == "/":
        filename = "index.html"

    return True, filename

def worker(addr, current_port, request):
    print addr, current_port
    print request

    sd_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sd_client.connect((addr[0],current_port))
    except socket.error, msg:
        print "Couldn't connect: %s\n" % msg
        return

    valid, filename = validate_http_request(request)

    if not valid:
        print "HTTP request is incorrect"
        return

    # I am assuming the Upload folder will be in
    # the same directory as this source file
    output = os.path.join("Upload", filename)
    if not os.path.isfile(output):
        print "File {0} doesn't exists".format(filename)
        return

    # read contents of file and write them to the client
    with open(output) as f:
        contents = f.read()
        sd_client.send(contents)
        sd_client.close()
        f.close()


def main():

    if len(sys.argv) != 2:
        print "Run: {0} [port]".format(sys.argv[0])
        return

    # input data needed
    port = int(sys.argv[1])
    if port == FIXED_PORT:
        print "port is reserved for establishing connections"
        return

    #host = socket.gethostname()
    # line below is here to test on my personal laptop
    # with no resolvable hostname
    host = '127.0.0.1'

    # create and bind socket
    sd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sd.bind((host, FIXED_PORT))
    sd.listen(5)

    # actively listen for requests
    threads = []
    while 1:

        conn, addr = sd.accept()
        data = conn.recv(BUFFER_SIZE)

        # if I did receive a message, proceed
        if data:
            t = threading.Thread(target=worker, args=[addr, port, data])
            threads.append(t)
            t.start()

        conn.close()

if __name__ == '__main__':
    main()
