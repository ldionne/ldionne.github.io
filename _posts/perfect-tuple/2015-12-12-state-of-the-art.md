---
layout: post
title: "The quest for a perfect tuple: The state of the art"
comments: true
tags: tuple metaprogramming
series: "The quest for a perfect tuple"
charts: true
---

In this post, we will look at the current state of the art for implementing
tuples at the library level in C++. More specifically, we will look at two
different techniques for implementing tuples, and we will (try to) analyze
them through the criteria that we established in the previous post:

1. Compile-time efficiency
2. Runtime efficiency
3. Compression of empty objects
4. `constexpr`-friendliness

{% include series.html %}

## First implementation technique: recursive atoms

The first implementation technique that we'll explore is pretty straightforward.
To my knowledge, this technique was first used in the original implementation
of [`boost::tuple`][Boost.Tuple] by Jaakko Järvi, several years ago. Basically,
it consists in implementing a tuple as a linked list of atoms, each atom
holding a single element of the tuple and a list of the remaining elements,
recursively:

{% highlight c++ %}
template <typename ...T>
struct tuple;

// An empty tuple has no members; this is the base case
template <>
struct tuple<> { };

// Otherwise, a tuple holds the first element and
// a tuple of the remaining elements
template <typename T, typename ...U>
struct tuple<T, U...> {
    T element;
    tuple<U...> rest;

    constexpr explicit tuple(T const& t, U const& ...u)
        : element(t), rest(u...)
    { }
};
{% endhighlight %}

This is fairly simple. Then, to access the n-th element of such a tuple, we
can write the following:

{% highlight c++ %}
template <std::size_t n, typename T, typename ...U>
constexpr decltype(auto) get(tuple<T, U...> const& ts) {
    return get<n-1>(ts.rest);
}

template <typename T, typename ...U>
constexpr T const& get<0>(tuple<T, U...> const& ts) {
    return ts.element;
}
{% endhighlight %}

Actually, we can't write the above as-is because function templates can't be
partially specialized. It is very easy to work around this limitation by using
a `struct` instead, but let's focus on the important things.

With our naive implementation, empty objects are not compressed. However, it
is pretty easy to change this by using the well-known technique of
[empty base optimization][ebo]:

{% highlight c++ %}
template <typename T, typename ...U>
struct tuple<T, U...> : tuple<U...>, element_holder<T> { };
{% endhighlight %}

In the above, `element_holder` is an imaginary template that uses EBO to reduce
the storage required for empty members to 0. An in-depth explanation of how
this might look like is explained on [K-ballo's blog][ebo-impl]. Of course,
that would require adjusting the implementation of the constructor and the
`get` function, but those adjustments are almost trivial. Hence, we can say
that with a little bit more work, this implementation strategy for tuples
would be able to compress the storage of empty objects.

Also, since we never use `constexpr`-unfriendly operations, we can mark our
functions as `constexpr`. Hence, the above tuple is a literal type, i.e. one
that can be constructed at compile-time.


## Leveling up: multiple inheritance

The second implementation technique that we'll explore is much less obvious
than the first one. I was first made aware of it by a post on the [Boost.Dev][]
mailing list by [Agustín Bergé][talesofcpp], but he told me that Howard Hinnant
had been using the technique in libc++ before that. Basically, the technique
consists in inheriting multiply from some base classes that hold the actual
elements of the tuple:

{% highlight c++ %}
template <typename ...T>
struct tuple : holder<0, T0>, holder<1, T1>, ..., holder<n-1, Tn-1> {
    // ...
};
{% endhighlight %}

Of course, this isn't valid C++. However, the same effect can be achieved by
using `std::index_sequence` and a bit of creativity:

{% highlight c++ %}
template <std::size_t i, typename T>
struct holder { T element; };

template <typename Indices, typename ...T>
struct tuple_impl;

template <std::size_t ...i, typename ...T>
struct tuple_impl<std::index_sequence<i...>, T...> : holder<i, T>... {
    constexpr explicit tuple_impl(T const& ...t)
        : holder<i, T>{t}...
    { }
};

template <typename ...T>
struct tuple : tuple_impl<std::make_index_sequence<sizeof...(T)>, T...> {
    // ...
};
{% endhighlight %}

While original, the above should not be too surprising. However, the very
clever part is in the implementation of the `get` function for such a tuple:

{% highlight c++ %}
template <std::size_t n, typename T>
constexpr T const& get(holder<n, T> const& t) {
    return t.element;
}
{% endhighlight %}

Yes, that's all! When we call `get<n>` on a tuple, the only function that
matches is the `get` overloaded function. However, since we're specifying
`n` explicitly, the compiler has to match the tuple against `holder<n, T>`
for some type `T`. Since there is exactly one such `holder` base class of
the tuple, the tuple gets implicitly converted to the right `holder`, which
holds the element you were looking for! Clever, huh? For a more in-depth
explanation of how this works, see [this other post][efficient_packing] by
K-ballo.

With the naive implementation shown above, empty objects are obviously not
compressed. However, it is decently easy to implement this optimization with
EBO, by using a trick similar to what we outlined above. Hence, we can say
that this representation of a tuple supports compressing empty objects.

Also, we did not use any `constexpr`-unfriendly operation in our implementation,
so the functions can be marked as `constexpr` and this tuple is a literal type.
Let's now take a look at the compile-time and runtime performance of these
two tuple implementations.


## Compile-time efficiency

Now's the time to get serious, because compile-time efficiency is an important
point. We'll compare the two above implementations on several aspects. First,
we'll want to measure the sheer cost of instantiating tuples implemented in
both ways. More specifically, we'll want to measure how well an implementation
scales in terms of the size of the tuple created, and how well it scales in
terms of the number of tuples instantiated (given a fixed tuple size). For
the first aspect, our benchmark code will look like this:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x1<0>, x1<1>, ..., x1<n>> t1;
    tuple<x2<0>, x2<1>, ..., x2<n>> t2;
    ...
    tuple<x10<0>, x10<1>, ..., x10<n>> t10;
}
{% endhighlight %}

The methodology is simply to measure the compile-time of such a file for various
values of `n`. The reason why we instantiate 10 different tuples every time is
that it reduces the relative incertitude on the measurement. We're almost ready
to look at the result, but before we do, let's pick a baseline to compare all
implementations against:

{% highlight c++ %}
template <typename ...T>
struct tuple {
    static constexpr std::size_t sizes[] = {sizeof(T)...};
};
{% endhighlight %}

This baseline is obviously not a valid tuple, but measuring it will still
allow us to factor out some noise such as compiler startup time. Because
of the `sizes` array, this will also allow us to factor out the cost of
instantiating the elements of the tuple themselves. Here's a chart
showing the results:

<!-- ninja -C _code/build tuple-quest.make_compile1 -->
<div class="chart" style="width:100%; height:400px;">
{
    "title": {
        "text": "Compile-time behavior of creating a tuple of length n"
    },
    "xAxis": {
        "title": { "text": "Size of the tuple" }
    },
    "series": [
        {
            "name": "atoms",
            "data": [[0, 0.32252791500650346], [10, 0.37476304097799584], [20, 0.40131123299943283], [30, 0.4464209059951827], [40, 0.4933795920223929], [50, 0.5875549199990928], [60, 0.6269187419675291], [70, 0.6898943550186232], [80, 0.8194551650085486], [90, 0.9071355229825713], [100, 1.0086682600085624], [110, 1.1469206880428828], [120, 1.2553649840410799], [130, 1.5358630630071275], [140, 1.5090981919784099], [150, 1.703226801007986], [160, 1.9030309419613332], [170, 1.967162771965377], [180, 2.213856263027992], [190, 2.436333558987826], [200, 2.9156332010170445], [210, 2.9746124299708754], [220, 3.126260496035684], [230, 3.410883233009372], [240, 3.6746486999909393], [250, 4.002902075997554], [260, 4.266632813960314], [270, 4.510958145954646], [280, 4.8864003959461115], [290, 5.1047923840233125], [300, 5.426421137002762]]
        }, {
            "name": "flat",
            "data": [[0, 0.3426869090180844], [10, 0.358175483008381], [20, 0.38664237497141585], [30, 0.3880674359970726], [40, 0.4023897360311821], [50, 0.4282817380153574], [60, 0.44382818799931556], [70, 0.48901676101377234], [80, 0.48097267298726365], [90, 0.5142624839791097], [100, 0.5294386360328645], [110, 0.5574877019971609], [120, 0.538554064987693], [130, 0.5688539200345986], [140, 0.5661929110065103], [150, 0.6144757879665121], [160, 0.6592116670217365], [170, 0.6111160799628124], [180, 0.6680068859714083], [190, 0.6932048629969358], [200, 0.710114841000177], [210, 0.7048033219762146], [220, 0.7328993339906447], [230, 0.7489577259984799], [240, 0.7632744809961878], [250, 0.7697581590036862], [260, 0.8236610610038042], [270, 0.8310004340019077], [280, 0.8514622179791331], [290, 0.9121377600240521], [300, 0.8761331599671394]]
        }, {
            "name": "baseline",
            "data": [[0, 0.3173864139826037], [10, 0.3448701009619981], [20, 0.35208114504348487], [30, 0.37898143799975514], [40, 0.34651065099751577], [50, 0.3599465360166505], [60, 0.3302134340046905], [70, 0.34170529595576227], [80, 0.35899486002745107], [90, 0.3350345470244065], [100, 0.3485674600233324], [110, 0.35728356504114345], [120, 0.3613241080311127], [130, 0.3543877280317247], [140, 0.366135440999642], [150, 0.36144322698237374], [160, 0.37152078200597316], [170, 0.3571894229971804], [180, 0.3637265790021047], [190, 0.3627612999989651], [200, 0.3734770200098865], [210, 0.36942556098802015], [220, 0.36978211399400607], [230, 0.36887982103507966], [240, 0.36594226001761854], [250, 0.37561082700267434], [260, 0.3829635260044597], [270, 0.37613581598270684], [280, 0.3821433869889006], [290, 0.3909721000236459], [300, 0.3858391069807112]]
        }
    ]
}
</div>

As we can see, the first implementation, which used recursive atoms, is much
worse than the flat implementation using multiple inheritance. Frankly, I'm
not sure why this is so. It could be due to the deeply nested instantiations,
or to the fact that the symbols are much longer with the first implementation.
Indeed, for the first implementation, we get something like:

{% highlight c++ %}
tuple<T1, T2, ..., Tn>
tuple<T2, ..., Tn>
...
tuple<Tn>
{% endhighlight %}

whereas for the second implementation, we only get

{% highlight c++ %}
tuple<T1, T2, ..., Tn>
tuple_impl<std::index_sequence<1, ..., n>, T1, T2, ..., Tn>
holder<1, T1>,
holder<2, T2>,
...
holder<n, Tn>
{% endhighlight %}

The trick here is that each `holder` is much, much shorter than a whole
`tuple<T1, ..., Tn>`. OK; measuring the instantiation cost of large tuples
is nice, but we'd also like to know how these techniques scales with respect
to the number of different tuples created. To do this, we'll benchmark the
following code:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<1>, x<2>, ..., x<10>> t1;
    tuple<x<2>, x<3>, ..., x<11>> t2;
    ...
    tuple<x<n>, x<n+1>, ..., x<n+10>> tn;
}
{% endhighlight %}

Here, we're arbitrarily setting what seems to be a sensible tuple size (`10`),
and then measuring the time required to instantiate `n` tuples of that size.
To make sure we're actually instantiating different tuple types, we shift the
types of the elements of each tuple by one. Here's the result:

<!-- ninja -C _code/build tuple-quest.make_compile2 -->
<div class="chart" style="width:100%; height:400px;">
{
    "title": {
        "text": "Compile-time behavior of creating n tuples"
    },
    "xAxis": {
        "title": { "text": "Number of tuples" }
    },
    "series": [
        {
            "name": "atoms",
            "data": [[0, 0.3031120580271818], [10, 0.3653895070310682], [20, 0.3663804469979368], [30, 0.4160121890017763], [40, 0.48540901800151914], [50, 0.4627230089972727], [60, 0.5016049690311775], [70, 0.5240902309888043], [80, 0.6763172719511203], [90, 0.6312445049989037], [100, 0.5739593970356509], [110, 0.8161111410008743], [120, 0.6685234369942918], [130, 0.687216994992923], [140, 0.6939177319873124], [150, 0.7567666880204342], [160, 0.7695203460170887], [170, 0.790973060997203], [180, 0.8090593600063585], [190, 0.8473627969506197], [200, 0.861952088016551], [210, 0.9241499800118618], [220, 0.9297847150010057], [230, 0.9293775880360045], [240, 0.9666079439921305], [250, 1.0096755369449966], [260, 1.0281325760297477], [270, 1.0731081779813394], [280, 1.0761376210139133], [290, 1.1271103780018166], [300, 1.1427464890293777], [310, 1.1736012870096602], [320, 1.283062532020267], [330, 1.3018969480181113], [340, 1.2870535270194523], [350, 1.287906554993242], [360, 1.3396281320019625], [370, 1.4118485749932006], [380, 1.3848495979909785], [390, 1.397247959044762], [400, 1.4028978670248762], [410, 1.4643996779923327], [420, 1.4600928179570474], [430, 1.4706959039904177], [440, 1.516849261999596], [450, 1.5531395740108564], [460, 1.5415422130026855], [470, 1.5580950480070896], [480, 1.6419800210278481], [490, 1.6143809199566022], [500, 1.6511281999992207], [510, 1.6897974150488153], [520, 1.7539129760116339], [530, 1.729574603959918], [540, 1.7476258620154113], [550, 1.7596089010476135], [560, 1.8188327830284834], [570, 1.806973465019837], [580, 1.8895473980228417], [590, 1.8508863700553775], [600, 1.920085659949109], [610, 1.9187599790166132], [620, 1.96463489998132], [630, 2.045713310013525], [640, 2.1547517760191113], [650, 2.041853467002511], [660, 2.060602924961131], [670, 2.0603292399900965], [680, 2.180070003028959], [690, 2.150757225987036], [700, 2.1556406739982776], [710, 2.2034219399793074], [720, 2.2149574200157076], [730, 2.219473010976799], [740, 2.259342950012069], [750, 2.3699592420016415], [760, 2.2791174600133672], [770, 2.4043478629901074], [780, 2.3755745860398747], [790, 2.3450493199634366], [800, 2.473180177039467], [810, 2.438572490995284], [820, 2.468189387000166], [830, 2.4874739289516583], [840, 2.543226151028648], [850, 2.6144432359724306], [860, 2.6147294939728454], [870, 2.5988590139895678], [880, 2.621091250970494], [890, 2.7830107850022614], [900, 2.7404620989691466], [910, 2.689856245997362], [920, 2.7983794530155137], [930, 2.8009829120128416], [940, 2.89835818100255], [950, 2.9801839170395397], [960, 2.9089086439926177], [970, 2.866982393024955], [980, 3.015905626001768], [990, 2.8851101450272836], [1000, 2.943300769024063]]
        }, {
            "name": "flat",
            "data": [[0, 0.3035969319753349], [10, 0.35280284099280834], [20, 0.3588720739935525], [30, 0.3718631910160184], [40, 0.3956142439856194], [50, 0.3982393640326336], [60, 0.44139183999504894], [70, 0.45461406698450446], [80, 0.48202635400230065], [90, 0.4746352629736066], [100, 0.5423542410135269], [110, 0.5323928029974923], [120, 0.5621551299700513], [130, 0.559556710999459], [140, 0.6444535960326903], [150, 0.6626547690248117], [160, 0.6788213399704546], [170, 0.7656754169729538], [180, 0.7094284429913387], [190, 0.6892591050127521], [200, 0.6974709029891528], [210, 0.7556893500150181], [220, 0.7991317060077563], [230, 0.8533393280231394], [240, 0.807190315972548], [250, 0.838762523024343], [260, 0.8401586819672957], [270, 0.8319102219538763], [280, 0.8774487859918736], [290, 0.9516298790113069], [300, 0.9398129060282372], [310, 0.8982577819842845], [320, 1.180792201019358], [330, 1.2335528330295347], [340, 0.9986743129556999], [350, 1.0898916290025227], [360, 1.1732244660379365], [370, 1.0598818520084023], [380, 1.304743446991779], [390, 1.2769993990077637], [400, 1.294550629972946], [410, 1.50194696104154], [420, 1.4188909910153598], [430, 1.392025517008733], [440, 1.2157786570023745], [450, 1.404339083994273], [460, 1.6086456860066392], [470, 1.7083140799659304], [480, 1.4177558480296284], [490, 1.4774268199689686], [500, 1.389347501040902], [510, 1.4276473129866645], [520, 1.4336227519670501], [530, 1.419385313987732], [540, 1.5613418080029078], [550, 1.4271076160366647], [560, 1.5823311380227096], [570, 1.6041379399830475], [580, 1.513132955005858], [590, 1.5117247099988163], [600, 1.5483277209568769], [610, 1.5631152130081318], [620, 1.5715593489585444], [630, 1.8973186610382982], [640, 1.7811646750196815], [650, 1.6139253129949793], [660, 1.7497029490186833], [670, 1.7307081070030108], [680, 1.7130129940342158], [690, 1.778794784040656], [700, 1.7225619929959066], [710, 1.753444944974035], [720, 1.8295395519817248], [730, 1.7550852840067819], [740, 1.8402185029699467], [750, 1.8081976710236631], [760, 1.8896475799847394], [770, 1.8668197999941185], [780, 2.852976612979546], [790, 2.14426937996177], [800, 2.513280968007166], [810, 2.655171369027812], [820, 2.1439982659649104], [830, 1.9881334459641948], [840, 2.0270148999989033], [850, 2.003310107975267], [860, 2.056692822952755], [870, 2.0623525209957734], [880, 2.168682110030204], [890, 2.2070061559788883], [900, 2.2365920119918883], [910, 2.3100736209889874], [920, 2.2451202520169318], [930, 2.4892667249660008], [940, 2.2557132429792546], [950, 2.278759097040165], [960, 2.3519169279607013], [970, 2.3015326560125686], [980, 2.361248436034657], [990, 2.318474013998639], [1000, 2.320779369038064]]
        }, {
            "name": "baseline",
            "data": [[0, 0.339961769990623], [10, 0.3317403080291115], [20, 0.34859660803340375], [30, 0.35557345900451764], [40, 0.3418363389791921], [50, 0.3859975960222073], [60, 0.33324928098591045], [70, 0.34088987100403756], [80, 0.3546873719897121], [90, 0.3655062449979596], [100, 0.3886123269912787], [110, 0.3565238149603829], [120, 0.3645397310028784], [130, 0.3796575710293837], [140, 0.3654494339716621], [150, 0.3654824969707988], [160, 0.3910401159664616], [170, 0.3837877929909155], [180, 0.36952036200091243], [190, 0.38688346702838317], [200, 0.39432710100663826], [210, 0.39895677496679127], [220, 0.3865940950345248], [230, 0.40775373799260706], [240, 0.3931310980115086], [250, 0.3820634000003338], [260, 0.40295062999939546], [270, 0.39897583797574043], [280, 0.4063293450162746], [290, 0.4220738990115933], [300, 0.4033049590070732], [310, 0.41226127598201856], [320, 0.4279181430465542], [330, 0.4477459369809367], [340, 0.4200506680062972], [350, 0.4478778539923951], [360, 0.4337574809906073], [370, 0.4184848179575056], [380, 0.4230365519761108], [390, 0.420814456010703], [400, 0.4431783679756336], [410, 0.45369356602896005], [420, 0.4492215330246836], [430, 0.4359656920423731], [440, 0.4562172410078347], [450, 0.4389222789905034], [460, 0.4714215449639596], [470, 0.45866704103536904], [480, 0.44705718103796244], [490, 0.4558724309899844], [500, 0.4568131620180793], [510, 0.4758070909883827], [520, 0.45399147301213816], [530, 0.46112434001406655], [540, 0.47063319699373096], [550, 0.46965623303549364], [560, 0.46198274998459965], [570, 0.521947311994154], [580, 0.47343422897392884], [590, 0.4890232650213875], [600, 0.5071831120294519], [610, 0.4954599749762565], [620, 0.4949009900446981], [630, 0.49250478303292766], [640, 0.48856463603442535], [650, 0.5193399239797145], [660, 0.4898098150151782], [670, 0.5035610710037872], [680, 0.5177035810193047], [690, 0.5040935500292107], [700, 0.5048389389994554], [710, 0.5247083660215139], [720, 0.5265502479742281], [730, 0.550859114038758], [740, 0.5126473440323025], [750, 0.5281792400055565], [760, 0.5285399770364165], [770, 0.516897686989978], [780, 0.5205814969958737], [790, 0.533727178000845], [800, 0.5278345469851047], [810, 0.5181446750066243], [820, 0.5464836829924025], [830, 0.5445926760439761], [840, 0.5445025290246122], [850, 0.5274145719595253], [860, 0.5748269410105422], [870, 0.5511641809716821], [880, 0.5504501039977185], [890, 0.5720450840308331], [900, 0.5523500379640609], [910, 0.5842247609980404], [920, 0.5223973130341619], [930, 0.5827394269872457], [940, 0.5471773630124517], [950, 0.593245875032153], [960, 0.5977135360008106], [970, 0.5742757769767195], [980, 0.6008248690050095], [990, 0.5718633940559812], [1000, 0.5727378409937955]]
        }
    ]
}
</div>

As we can see, both implementations perform in a similar fashion, the flat
tuple being slightly better. This is most likely due to the same reasons as
the previous benchmark, except the difference is less exacerbated because the
created tuples are smaller.

Now, creating tuples is fun, but accessing elements in a tuple is more fun.
Hence, we'll also want to measure how well our implementations scale when
accessing elements with the `get` function. Much like the previous benchmarks,
there are mainly two interesting access patterns we can benchmark. The first
one is to fix a tuple size, and to access varying numbers of elements inside
the tuple. This pattern can be measured with the following benchmark:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<0>, x<1>, ..., x<500>> t;
    get<0 * (500/n)>(t);
    get<1 * (500/n)>(t);
    ...
    get<n * (500/n)>(t);
}
{% endhighlight %}

Here, we arbitrarily set the size of the tuple to `500`, which is large enough,
and then access `n` elements inside the tuple. Note that the `n` elements that
we access are placed at constant intervals, and they are almost uniformly
distributed in the \\([1, 500]\\) interval. We do this to avoid accessing the
first (or last) `n` elements, which would favor an implementation where
accessing leading (or trailing) elements is faster. Also, while not shown
here, we actually create a tuple of each implementation and only access
elements of the implementation we're benchmarking. This is to factor out
the initial cost of creating the tuple, which varies between implementations.
Before we show the results, let's show the baseline that we'll use:

{% highlight c++ %}
template <std::size_t i, typename Tuple>
constexpr int get(Tuple const&) {
    return 0;
}
{% endhighlight %}

Again, this is obviously not a correct implementation of `get`, but it will at
least allow us to factor out the cost of instantiating the `get` function. Here
is the result:

<!-- ninja -C _code/build tuple-quest.get_compile1 -->
<div class="chart" style="width:100%; height:400px;">
{
    "title": {
        "text": "Compile-time behaviour of getting n elements in a tuple"
    },
    "xAxis": {
        "title": {
            "text": "Number of elements"
        },
        "minTickInterval": 1
    },
    "series": [
        {
            "name": "atoms",
            "data": [[1, 2.7430356850018143], [51, 19.080829823000386], [101, 33.69522942399999], [151, 53.777618243999314], [201, 65.26296033999824], [251, 56.87080646599861], [301, 79.05052655300096], [351, 104.25828618499872], [401, 133.1576100880011], [451, 164.38040564499897]]
        }, {
            "name": "flat",
            "data": [[1, 1.8168098329988425], [51, 1.7710727900011989], [101, 1.8380601889984973], [151, 1.8823173430027964], [201, 1.973657924998406], [251, 1.9703010929988523], [301, 2.029322365000553], [351, 2.066871686001832], [401, 2.1442172110000683], [451, 2.1901850509966607]]
        }, {
            "name": "baseline",
            "data": [[1, 1.7749120709995623], [51, 1.7713077929984138], [101, 1.829411581999011], [151, 1.8837151599982462], [201, 1.91652816700298], [251, 1.964494175001164], [301, 2.0011897389995283], [351, 2.0330733400005556], [401, 2.0754479209972487], [451, 2.1292497419999563]]
        }
    ]
}
</div>

Here, we can see that the recursive tuple implementation is really, really awful.
Indeed, every time we want to get an element, we need to recurse down to that
element in our chain of base classes. In contrast, the flat tuple implementation
only requires instantiating a single function every time we get an element,
which makes it O(1).

The second access pattern that we might want to measure is to fix a number of
elements to access inside the tuple, and to let the indices of these elements
grow larger and larger. This access pattern can be measured with the following
benchmark:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<0>, x<1>, ..., x<500>> t;
    get<n>(t);
    get<n+1>(t);
    get<n+2>(t);
    get<n+3>(t);
    get<n+4>(t);
    get<n+5>(t);
    get<n+6>(t);
    get<n+7>(t);
    get<n+8>(t);
    get<n+9>(t);
}
{% endhighlight %}

Here, we decide to access exactly `10` elements in the tuple. We then let the
index of these elements vary. The result is:

<!-- ninja -C _code/build tuple-quest.get_compile2 -->
<div class="chart" style="width:100%; height:400px;">
{
    "title": {
        "text": "Compile-time behaviour of getting the n-th element of a tuple"
    },
    "xAxis": {
        "title": {
            "text": "Index"
        },
        "minTickInterval": 1
    },
    "series": [
        {
            "name": "atoms",
            "data": [[0, 1.9426959510019515], [50, 2.8110494120010117], [100, 3.670933094002976], [150, 4.405426904999331], [200, 5.076616256999841], [250, 5.58531480199963], [300, 6.079452178000793], [350, 6.581843496001966], [400, 7.850460663998092], [450, 8.305800688998715]]
        }, {
            "name": "flat",
            "data": [[0, 2.1213851860011346], [50, 2.1233816300009494], [100, 2.335437176996493], [150, 1.8210540569998557], [200, 1.8032461750008224], [250, 1.79683842400118], [300, 1.8372620909976831], [350, 2.699952344999474], [400, 2.4300092769990442], [450, 1.9091410359978909]]
        }, {
            "name": "baseline",
            "data": [[0, 2.0268621840004926], [50, 1.9655956659989897], [100, 2.0171332089994394], [150, 1.98518869200052], [200, 1.9271590069984086], [250, 1.8430860329972347], [300, 2.2936138969998865], [350, 2.1540698899989366], [400, 2.0894923609994294], [450, 1.899316942999576]]
        }
    ]
}
</div>

As expected, we can see that the recursive implementation is worse than the flat
implementation. Furthermore, whereas the index at which an element is accessed
does not seem to matter for the flat implementation, the recursive implementation
is slower when accessing elements near the end of the tuple. Again, this is
because the recursive implementation must walk the whole tuple up to the
requested index, whereas the flat implementation performs in O(1) no matter
the index.


## Runtime efficiency

In theory, both implementations only rely on the compiler being able to see
through base classes, `static_cast`s and inline functions. Hence, they should
cause the compiler to generate equivalent code, right? That's what we'll see.
To check this, we'll look at the assembly generated when accessing the elements
of a tuple at different indices:

{% highlight c++ %}
int main() {
    tuple<int, ..., int> t{0, 1, ..., n};
    get<0>(t);
    get<1>(t);
    ...
    get<n>(t);
}
{% endhighlight %}

At the same time, we'll compare it with the assembly generated for equivalent
code using a `struct` instead of a `tuple`. We'll assume that the `struct` is
optimized as much as possible by the compiler (otherwise that's arguably a
compiler bug), and use this as our baseline:

{% highlight c++ %}
struct Baseline {
    int element_0;
    int element_1;
    ...
    int element_n;
};

int main() {
    Baseline t{0, 1, ..., n};
    t.element_0;
    t.element_1;
    ...
    t.element_n;
}
{% endhighlight %}

To make sure the optimizer does not get rid of the code altogether, we'll use
the `escape()` trick described by Chandler Carruth in [this talk][Chandler.micro]
about microbenchmarks. Then, since we don't want to actually read a bunch of
repetitive assembly, we'll only compare the number of lines in the assembly
produced by each of the above:

<!-- TODO: tuple-quest/get_assembly/chart.json -->
<div class="chart" style="width:100%; height:400px;">
TODO
</div>

<!-- TODO: comment -->

### The NRVO test

A very important optimization in C++ is [NRVO][]. Basically, NRVO is when the
compiler constructs a local variable returned by value in the caller instead
of creating it in the caller and then copying it into the callee. Since this
is an important optimization, and since I've already seen some disturbing cases
where NRVO wasn't applied but it should have been, let's look at how our tuple
implementations play with this optimization. To achieve this, we'll create a
tuple with many elements, and we'll return it by value from a function.
Normally, the tuple should be constructed in-place and never copied. Here's
the code:

{% highlight c++ %}
template <int i>
struct x {
    x() { std::cout << "x()" << std::endl; }
    x(x const&) { std::cout << "x(x const&)" << std::endl; }
};

auto f() {
    tuple<x<0>, x<1>, ..., x<n>> t;
    return t; // should NRVO
}

int main() {
    auto t = f();
}
{% endhighlight %}

Let's now run this code with different values of `n`, and see if NVRO is
applied:

<!-- TODO: tuple-quest/nrvo/table.md -->

<!-- TODO: comment -->

## Summary

In summary, we can see that the second implementation is vastly superior to
the first, naive one. Not only is it better in terms of compile-time, but it
also causes the compiler to generate superior code, which is quite important.

|                      | Compile-time | Runtime | Compression | `constexpr` |
| -------------------- | :----------: | :-----: | :---------: | :---------: |
| recursive atoms      | -            | -       |             | -           |
| multiple inheritance | -            | -       |             | -           |

<!-- TODO -->

<!-- Links -->
[Boost.Dev]: http://news.gmane.org/gmane.comp.lib.boost.devel
[Boost.Tuple]: http://www.boost.org/doc/libs/release/libs/tuple/doc/tuple_users_guide.html
[Chandler.micro]: https://youtu.be/nXaxk27zwlk?t=2527
[ebo-impl]: http://talesofcpp.fusionfenix.com/post-18/episode-ten-when-size-does-matter
[ebo]: http://en.cppreference.com/w/cpp/language/ebo
[efficient_packing]: http://talesofcpp.fusionfenix.com/post-22/true-story-efficient-packing#the-case-of-the-`tuple`
[NRVO]: https://en.wikipedia.org/wiki/Return_value_optimization
[talesofcpp]: http://talesofcpp.fusionfenix.com
