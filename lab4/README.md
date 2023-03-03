```bash
sudo pkill ovs-test
sudo python3 topo.py
```

Pachetele primite/trimise de router vor avea urmatorul format:

```
typedef struct {
    int len;
    char payload[MAX_LEN];
    int interface;
} msg;
```

In implementarea laboratorului vom folosii urmatoarele functii: **get\_packet**
care primeste un pachet si **send\_packet** care trimite un pachet pe o
interfata specificata.

```
/* Blocking function, blocks if there is no packet to be received.
On success returns 0 and populates m. m must be allocated! */
int get_packet(msg *m);
int send_packet(int interface, msg *m);
```
