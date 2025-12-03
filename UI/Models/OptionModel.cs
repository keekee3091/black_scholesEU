using System.Text.Json.Serialization;

public class OptionModel
{
    [JsonPropertyName("type")]
    public string Type { get; set; }

    [JsonPropertyName("strike")]
    public double Strike { get; set; }

    [JsonPropertyName("spot")]
    public double Spot { get; set; }

    [JsonPropertyName("expiration")]
    public string Expiration { get; set; }

    [JsonPropertyName("maturity")]
    public double Maturity { get; set; }

    [JsonPropertyName("sigma")]
    public double Sigma { get; set; }

    [JsonPropertyName("bs_price")]
    public double BSPrice { get; set; }

    [JsonPropertyName("market_price")]
    public double MarketPrice { get; set; }

    [JsonPropertyName("mispricing")]
    public double Mispricing { get; set; }

    [JsonPropertyName("delta")]
    public double Delta { get; set; }

    [JsonPropertyName("gamma")]
    public double Gamma { get; set; }

    [JsonPropertyName("theta")]
    public double ThetaBleed { get; set; }

    [JsonPropertyName("vega")]
    public double Vega { get; set; }

    [JsonPropertyName("rho")]
    public double Rho { get; set; }

    [JsonPropertyName("prob_ITM")]
    public double ProbITM { get; set; }

    [JsonPropertyName("moneyness")]
    public double Moneyness { get; set; }

    [JsonPropertyName("iv_mean")]
    public double IVMean { get; set; }

    [JsonPropertyName("iv_std")]
    public double IVStd { get; set; }

    [JsonPropertyName("iv_z")]
    public double IVZ { get; set; }

    [JsonPropertyName("vega_score")]
    public double VegaNorm { get; set; }

    [JsonPropertyName("liquidity")]
    public double LiquidityScore { get; set; }

    [JsonPropertyName("gamma_risk")]
    public double GammaRisk { get; set; }

    [JsonPropertyName("final_score")]
    public double EnhancedScore { get; set; }

    [JsonPropertyName("action")]
    public string Action { get; set; }

    [JsonPropertyName("action_reason")]
    public string ActionReason { get; set; }
}
