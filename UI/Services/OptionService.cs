using UI.Models;

namespace UI.Services
{
    public class OptionService
    {
        private readonly HttpClient _httpClient;

        public OptionService(HttpClient httpClient)
        {
            _httpClient = httpClient;
        }

        public async Task<List<OptionModel>> GetOptions(string symbol, double r)
        {
            var endpoint = $"http://localhost:8080/price?symbol={symbol}&r={r}";
            var response = await _httpClient.GetFromJsonAsync<ApiResponse>(endpoint);

            return response?.Options?[symbol] ?? new List<OptionModel>() ;

        }
    }
}
