COMPONENT_ADD_INCLUDEDIRS = .
COMPONENT_OBJS :=  main.o \
	wifi_connect.o \
	sntp_connect.o \
	task_icmp_client.o \
	task_publish.o \
	task_ota.o
COMPONENT_EMBED_TXTFILES := cert.pem \
	certs/ca_cert_ota.pem
