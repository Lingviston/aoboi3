def get_best_cache_block_index(cache, cache_line_number, cache_line_number_block):
  best = None
  best_index = 0
  index = 0
  for block in cache:
    cache_line = block[cache_line_number_block]
    line_number = cache_line[0]
    use_count = cache_line[1]
    
    if line_number == cache_line_number: # cache hit
      best = cache_line
      best_index = index
      break
    elif line_number == None and use_count == None: # found empty cache line
      best = cache_line
      best_index = index
    elif best == None: # found cache_line and we haven't selected any other yet
      best = cache_line
      best_index = index
    elif best[1] != None and use_count < best[1]: # found cache line which is older than the one that we selected
      best = cache_line
      best_index = index
    
    index += 1
  
  return best_index


config_input = input()
(cache_size, associativity, line_size, n) = (int(cache_param) for cache_param in config_input.split(" "))

addresses_input = input()
addresses = [int(address) for address in addresses_input.split()]

block_lines_count = cache_size // line_size // associativity
cache = [[(None, None) for index in range(0, block_lines_count)] for block in range(0, associativity)]

cache_misses = 0
cache_hits = 0
index = 0
for address in addresses:
  index += 1
  cache_line_number = address // line_size
  cache_line_number_block = cache_line_number % block_lines_count
  
  block_index = get_best_cache_block_index(cache, cache_line_number, cache_line_number_block)
  
  if cache[block_index][cache_line_number_block][0] == cache_line_number:
    cache_hits += 1
  else:
    cache_misses += 1
  
  cache[block_index][cache_line_number_block] = (cache_line_number, index)

print(cache_hits, cache_misses)
    