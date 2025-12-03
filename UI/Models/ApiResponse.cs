namespace UI.Models
{
    public class ApiResponse
    {
        public string Symbol { get; set; }
        public Dictionary<string, List<OptionModel>> Options { get; set; }
    }

}
