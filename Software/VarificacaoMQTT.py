import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker: " + str(rc))
    client.subscribe("dados_digital")
    

def on_message(client, userdata, msg):
    if msg.topic == "dados_digital" and msg.payload.decode() == "1":
        client.publish("nome_va_teste", "Kaique")
        client.publish("dados_va_teste", "Teste")
        print("A mensagem foi enviada")

    if msg.topic == "dados_digital" and msg.payload.decode() == "51":
        client.publish("nome_va_teste", "Kaique")
        client.publish("dados_va_teste", "Teste 2 =)")
        print("A mensagem foi enviada")
               
client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message

client.connect("mqtt.eclipseprojects.io", 1883, 60)


client.loop_forever()