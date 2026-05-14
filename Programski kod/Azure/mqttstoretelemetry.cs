using Azure.Messaging.ServiceBus;
using Microsoft.Azure.Functions.Worker;
using Microsoft.Extensions.Logging;
using Azure.Data.Tables;
using Newtonsoft.Json;
using System.Text;
using Microsoft.Net.Http.Headers;
using Azure;
using System.Drawing.Drawing2D;
using Microsoft.Azure.Functions.Worker.Extensions.Abstractions;

namespace mqttamsqueue
{
    public class readtelemetrydata
    {
        private readonly ILogger<readtelemetrydata> _logger;
        private readonly string account_name = "mqtttelemetrydata";
        private readonly string storageUri = "https://mqtttelemetrydata.table.core.windows.net";
        private readonly string storageAccountKey = "VgcVM1W2Z5X9H/E/CFtqAUHzLeiFCGb5mul2SsIPMhyDlAMYyMs6bUlRXYRX9u97d1I96UVUdEzk+AStVaO0kA==";
        private readonly string table_name = "mqtttelemetrytable";
        private static int rowKey = 1;

        public readtelemetrydata(ILogger<readtelemetrydata> logger)
        {
            _logger = logger;
        }

        [Function(nameof(readtelemetrydata))]
        public async Task Run(
            [ServiceBusTrigger("mqtt_telemetry_queue", Connection = "mqttamsqueue_SERVICEBUS")]
            ServiceBusReceivedMessage message,
            ServiceBusMessageActions messageActions)
        {
            try{

                _logger.LogInformation("Message ID: {id}", message.MessageId);
                _logger.LogInformation("Message Body: {body}", message.Body);
                _logger.LogInformation("Message Content-Type: {contentType}", message.ContentType);

                string jsonString = Encoding.UTF8.GetString(message.Body);

                var JData = JsonConvert.DeserializeObject<JSONData>(jsonString);

                _logger.LogInformation("{id}, {source}, {type}, {data_base64}, {time}, {specversion}, {datacontenttype}, {subject}", JData.id, JData.source, JData.type, JData.data_base64, JData.time, JData.specversion, JData.datacontenttype, JData.subject);

                byte[] data = Convert.FromBase64String(JData.data_base64);

                string decodedString = Encoding.UTF8.GetString(data);

                string[] strings = decodedString.Split("|");

                ParsedData pData = new(float.Parse(strings[0]), Int32.Parse(strings[1]), " ");

                pData.time_data = JData.time.Split("T")[1].Remove(5);              

                var serviceClient = new TableServiceClient(
                    new Uri(storageUri),
                    new TableSharedKeyCredential(account_name, storageAccountKey));

                var tableClient = new TableClient(
                    new Uri(storageUri),
                    table_name,
                    new TableSharedKeyCredential(account_name, storageAccountKey));

                Pageable<TableEntity> exReading = tableClient.Query<TableEntity>(x=> x.RowKey == rowKey.ToString());

                if(exReading.Count() != 0)
                {
                    tableClient.DeleteEntity(partitionKey: "data", rowKey: rowKey.ToString());
                }

                var tableEntity = new TableEntity("data", rowKey.ToString()){
                    {"Temperature", pData.temperature_data},
                    {"Turbidity", pData.turbidity_data},
                    {"Time", pData.time_data}
                };

                tableClient.AddEntity(tableEntity);

                rowKey += 1;

                _logger.LogInformation("Temperature data: {temperature_data}", pData.temperature_data);
                _logger.LogInformation("Turbidity data: {turbidity_data}", pData.turbidity_data);
                _logger.LogInformation("Time : {time}", pData.time_data);
                
            } 
            catch (Exception ex)
            {
                switch(ex) 
                {
                    case RequestFailedException:
                        _logger.LogInformation("Entry with credentials \"PartitionKey:data, RowKey:{rowKey}\" already exists", rowKey);

                        //either free table at that row key or add one to key and try again
                        break;
                    case ArgumentNullException or FormatException:
                        _logger.LogInformation("Failed parsing sent data. Something wrong with data string. Do not retry!");
                        break;
                    default:
                        _logger.LogInformation("Unexpected error occured, do not retry!");
                        break;
                }
            }
            // Complete the message
            await messageActions.CompleteMessageAsync(message);
        }
    }

    class JSONData {
        required public string id { get; set;}
        required public string source { get; set;}
        required public string type { get; set; }
        required public string data_base64 { get; set; }
        required public string time { get; set; }
        required public string specversion { get; set; }
        required public string datacontenttype { get; set; }
        required public string subject { get; set; }
    }

    class ParsedData(float temperature_data, int turbidity_data, string time_data) {
        public float temperature_data { get; set; } = temperature_data;
        public int turbidity_data { get; set; } = turbidity_data;
        public string time_data { get; set; } = time_data;
    }
}
