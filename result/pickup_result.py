with open('result_all_retry.txt', 'r') as f:
    data = f.read().splitlines()

set_data = set([])

for datum in data:
    if datum[:8] == 'SOLUTION':
        set_data.add(datum.split()[1])

print(len(set_data))

with open('result_all_pickup.txt', 'w') as f:
    f.write(str(len(set_data)) + '\n')
    for elem in set_data:
        f.write(elem + '\n')

'''
set_data2 = set([])

for elem in set_data:
    if elem[:2] == 'f5':
        set_data2.add(elem)
    elif elem[:2] == 'e6':
        transcript = ''
        for i in range(0, len(elem), 2):
            x = ord(elem[i]) - ord('a')
            y = int(elem[i + 1]) - 1
            transcript += chr(ord('a') + y) + str(x + 1)
        print(elem, transcript)
        set_data2.add(transcript)
    elif elem[:2] == 'd3':
        transcript = ''
        for i in range(0, len(elem), 2):
            x = ord(elem[i]) - ord('a')
            y = int(elem[i + 1]) - 1
            transcript += chr(ord('a') + 7 - y) + str(7 - x + 1)
        print(elem, transcript)
        set_data2.add(transcript)
    elif elem[:2] == 'c4':
        transcript = ''
        for i in range(0, len(elem), 2):
            x = ord(elem[i]) - ord('a')
            y = int(elem[i + 1]) - 1
            transcript += chr(ord('a') + 7 - x) + str(7 - y + 1)
        print(elem, transcript)
        set_data2.add(transcript)


print(len(set_data2))

with open('result_all_pickup.txt', 'w') as f:
    f.write(str(len(set_data2)) + '\n')
    for elem in set_data2:
        f.write(elem + '\n')
'''