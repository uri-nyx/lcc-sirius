
pair: ADDI8(pair,pair)   ";ADDI8\n\tadd x%c^, x%0^, x%1^\n\tsltu x5, x%c^, x%0^\n\tadd x%c, x%0, x%1\n\tadd x%c, x%c, x5\n" 1
pair: ADDU8(pair,pair)   ";ADDU8\n\tadd x%c^, x%0^, x%1^\n\tsltu x5, x%c^, x%0^\n\tadd x%c, x%0, x%1\n\tadd x%c, x%c, x5\n" 1
pair: SUBI8(pair,pair)   ";SUBI8\n\tsltu x5, x%0^, x%1^\n\tsub x%c^, x%0^, x%1^\n\tsub x%c, x%0, x%1\n\tsub x%c, x%c, x5\n" 1
pair: SUBU8(pair,pair)   ";SUBU8\n\tsltu x5, x%0^, x%1^\n\tsub x%c^, x%0^, x%1^\n\tsub x%c, x%0, x%1\n\tsub x%c, x%c, x5\n" 1
pair: BANDI8(pair,pair)  ";BANDI8\n\tand x%c^, x%0^, x%1^\n\tand x%c, x%0, x%1\n" 1
pair: BANDU8(pair,pair)  ";BANDU8\n\tand x%c^, x%0^, x%1^\n\tand x%c, x%0, x%1\n" 1
pair: BORI8(pair,pair)   ";BORI8\n\tor x%c^, x%0^, x%1^\n\tor x%c, x%0, x%1\n" 1
pair: BORU8(pair,pair)   ";BORU8\n\tor x%c^, x%0^, x%1^\n\tor x%c, x%0, x%1\n" 1
pair: BXORI8(pair,pair)  ";BXORI8\n\txor x%c^, x%0^, x%1^\n\txor x%c, x%0, x%1\n" 1
pair: BXORU8(pair,pair)  ";BXORU8\n\txor x%c^, x%0^, x%1^\n\txor x%c, x%0, x%1\n" 1
pair: BCOMI8(pair)       ";BCOMI8\n\tnot x%c^, x%0^\n\tnot x%c, x%0\n" 1
pair: BCOMU8(pair)       ";BCOMU8\n\tnot x%c^, x%0^\n\tnot x%c, x%0\n" 1
pair: NEGI8(pair)        ";NEGI8\n\tsltu x5, x0, x%0^\n\tsub x%c^, x0, x%0^\n\tsub x%c, x0, x%0\n\tsub x%c, x%c, x5\n" 1

pair: ADDI8(pair,pair)   "\tcall __add64\n" 1
pair: ADDU8(pair,pair)   "\tcall __add64\n" 1
pair: SUBI8(pair,pair)   "\tcall __sub64\n" 1
pair: SUBU8(pair,pair)   "\tcall __sub64\n" 1
pair: BANDI8(pair,pair)  "\tcall __and64\n" 1
pair: BANDU8(pair,pair)  "\tcall __and64\n" 1
pair: BORI8(pair,pair)   "\tcall __or64\n" 1
pair: BORU8(pair,pair)   "\tcall __or64\n" 1
pair: BXORI8(pair,pair)  "\tcall __xor64\n" 1
pair: BXORU8(pair,pair)  "\tcall __xor64\n" 1
pair: BCOMI8(pair)       "\tcall __not64\n" 1
pair: BCOMU8(pair)       "\tcall __not64\n" 1
pair: NEGI8(pair)        "\tcall __neg64\n" 1
