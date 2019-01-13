import sys, json;

j = json.load(sys.stdin);
stats = [{'player': k, 'rank': v['rank'], 'score': v['score']} for k, v in j['stats'].items() if j['terminated'][k] is False]
stats.sort(key = lambda p: p['rank'])
print(json.dumps(stats))
